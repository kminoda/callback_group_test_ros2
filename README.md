# ndt_scan_matcher

## Purpose

ndt_scan_matcher is a package for position estimation using the NDT scan matching method.

There are two main functions in this package:

- estimate position by scan matching
- estimate initial position via the ROS service using the Monte Carlo method

One optional function is regularization. Please see the regularization chapter in the back for details. It is disabled by default.

## Inputs / Outputs

### Input

| Name                                | Type                                            | Description                           |
| ----------------------------------- | ----------------------------------------------- | ------------------------------------- |
| `ekf_pose_with_covariance`          | `geometry_msgs::msg::PoseWithCovarianceStamped` | initial pose                          |
| `pointcloud_map`                    | `sensor_msgs::msg::PointCloud2`                 | map pointcloud                        |
| `points_raw`                        | `sensor_msgs::msg::PointCloud2`                 | sensor pointcloud                     |
| `sensing/gnss/pose_with_covariance` | `sensor_msgs::msg::PoseWithCovarianceStamped`   | base position for regularization term |

> `sensing/gnss/pose_with_covariance` is required only when regularization is enabled.

### Output

| Name                              | Type                                            | Description                                                                                                                              |
| --------------------------------- | ----------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `ndt_pose`                        | `geometry_msgs::msg::PoseStamped`               | estimated pose                                                                                                                           |
| `ndt_pose_with_covariance`        | `geometry_msgs::msg::PoseWithCovarianceStamped` | estimated pose with covariance                                                                                                           |
| `/diagnostics`                    | `diagnostic_msgs::msg::DiagnosticArray`         | diagnostics                                                                                                                              |
| `points_aligned`                  | `sensor_msgs::msg::PointCloud2`                 | [debug topic] pointcloud aligned by scan matching                                                                                        |
| `initial_pose_with_covariance`    | `geometry_msgs::msg::PoseWithCovarianceStamped` | [debug topic] initial pose used in scan matching                                                                                         |
| `exe_time_ms`                     | `tier4_debug_msgs::msg::Float32Stamped`         | [debug topic] execution time for scan matching [ms]                                                                                      |
| `transform_probability`           | `tier4_debug_msgs::msg::Float32Stamped`         | [debug topic] score of scan matching                                                                                                     |
| `iteration_num`                   | `tier4_debug_msgs::msg::Int32Stamped`           | [debug topic] number of scan matching iterations                                                                                         |
| `initial_to_result_distance`      | `tier4_debug_msgs::msg::Float32Stamped`         | [debug topic] distance difference between the initial point and the convergence point [m]                                                |
| `initial_to_result_distance_old`  | `tier4_debug_msgs::msg::Float32Stamped`         | [debug topic] distance difference between the older of the two initial points used in linear interpolation and the convergence point [m] |
| `initial_to_result_distance_new`  | `tier4_debug_msgs::msg::Float32Stamped`         | [debug topic] distance difference between the newer of the two initial points used in linear interpolation and the convergence point [m] |
| `ndt_marker`                      | `visualization_msgs::msg::MarkerArray`          | [debug topic] markers for debugging                                                                                                      |
| `monte_carlo_initial_pose_marker` | `visualization_msgs::msg::MarkerArray`          | [debug topic] particles used in initial position estimation                                                                              |

### Service

| Name            | Type                                                         | Description                      |
| --------------- | ------------------------------------------------------------ | -------------------------------- |
| `ndt_align_srv` | `autoware_localization_srvs::srv::PoseWithCovarianceStamped` | service to estimate initial pose |

## Parameters

### Core Parameters

| Name                                    | Type   | Description                                                                                     |
| --------------------------------------- | ------ | ----------------------------------------------------------------------------------------------- |
| `base_frame`                            | string | Vehicle reference frame                                                                         |
| `input_sensor_points_queue_size`        | int    | Subscriber queue size                                                                           |
| `ndt_implement_type`                    | int    | NDT implementation type (0=PCL_GENERIC, 1=PCL_MODIFIED, 2=OMP)                                  |
| `trans_epsilon`                         | double | The maximum difference between two consecutive transformations in order to consider convergence |
| `step_size`                             | double | The newton line search maximum step length                                                      |
| `resolution`                            | double | The ND voxel grid resolution [m]                                                                |
| `max_iterations`                        | int    | The number of iterations required to calculate alignment                                        |
| `converged_param_transform_probability` | double | Threshold for deciding whether to trust the estimation result                                   |
| `omp_neighborhood_search_method`        | int    | neighborhood search method in OMP (0=KDTREE, 1=DIRECT26, 2=DIRECT7, 3=DIRECT1)                  |
| `omp_num_threads`                       | int    | Number of threads used for parallel computing                                                   |

## Regularization

### Abstract

This is a function that adds the regularization term to the NDT optimization problem as follows.

$$
\begin{align}
    \min_{\mathbf{R},\mathbf{t}}
    \mathrm{NDT}(\mathbf{R},\mathbf{t})
    +\mathrm{scale\ factor}\cdot \left|
        \mathbf{R}^\top
        (\mathbf{t_{base}-\mathbf{t}})
        \cdot
        \begin{pmatrix}
            1\\
            0\\
            0
        \end{pmatrix}
        \right|^2
\end{align}
$$

, where t_base is base position measured by GNSS or other means.
NDT(R,t) stands for the pure NDT cost function.
The regularization term shifts the optimal solution to the base position in the longitudinal direction of the vehicle.
Only errors along the **longitudinal direction** with respect to the base position are considered; errors along Z-axis and lateral-axis error are not considered.

Although the regularization term has rotation as a parameter, the gradient and hessian associated with it is not computed to stabilize the optimization.
Specifically, the gradients are computed as follows.

$$
\begin{align}
    &g_x=\nabla_x \mathrm{NDT}(\mathbf{R},\mathbf{t}) + 2 \mathrm{scale\ factor} \cos\theta_z\cdot e_{\mathrm{longitudinal}}
    \\
    &g_y=\nabla_y \mathrm{NDT}(\mathbf{R},\mathbf{t}) + 2 \mathrm{scale\ factor} \sin\theta_z\cdot e_{\mathrm{longitudinal}}
    \\
    &g_z=\nabla_z \mathrm{NDT}(\mathbf{R},\mathbf{t})
    \\
    &g_\mathbf{R}=\nabla_\mathbf{R} \mathrm{NDT}(\mathbf{R},\mathbf{t})
\end{align}
$$

Regularization is disabled by default.
If you wish to use it, please edit the following parameters to enable it.
Regularization is only available for `NDT_OMP` and not for other NDT implementation types (`PCL_GENERIC`, `PCL_MODIFIED`).

#### Where is regularization available

This feature is effective on feature-less roads where GNSS is available, such as

- bridges
- highways
- farm roads

By remapping the base position topic to something other than GNSS, as described below, it can be valid outside of these.

#### Using other base position

Other than GNSS, you can give other global position topics obtained from magnetic markers, visual markers or etc. if they are available in your environment.
(Currently Autoware does not provide a node that gives such pose.)
To use your topic for regularization, you need to remap the `input_regularization_pose_topic` with your topic in `ndt_scan_matcher.launch.xml`.
By default, it is remapped with `/sensing/gnss/pose_with_covariance`.

#### Limitations

Since this function determines the base position by linear interpolation from the recently subscribed poses,
topics that are published at a low frequency relative to the driving speed cannot be used.
Inappropriate linear interpolation may result in bad optimization results.

When using GNSS for base location, the regularization can have negative effects in tunnels, indoors, and near skyscrapers.
This is because if the base position is far off from the true value, NDT scan matching may converge to inappropriate optimal position.

### Parameters

| Name                          | Type   | Description                                                            |
| ----------------------------- | ------ | ---------------------------------------------------------------------- |
| `regularization_enabled`      | bool   | Flag to add regularization term to NDT optimization (FALSE by default) |
| `regularization_scale_factor` | double | Coefficient of the regularization term.                                |

Regularization is disabled by default because GNSS is not always accurate enough to serve the appropriate base position in any scenes.

If the scale_factor is too large, the NDT will be drawn to the base position and scan matching may fail.
Conversely, if it is too small, the regularization benefit will be lost.

Note that setting scale_factor to 0 is equivalent to disabling regularization.

### Example

The following figures show tested maps.

- The left is a map with enough features that NDT can successfully localize.
- The right is a map with so few features that the NDT cannot localize well.

<img src="./media/bridge_map.jpg" alt="drawing" width="300"/> <img src="./media/bridge_map_less_feature.jpg" alt="drawing" width="300"/>

The following figures show the trajectories estimated on the feature-less map with standard NDT and regularization-enabled NDT, respectively.
The color of the trajectory indicates the error (meter) from the reference trajectory, which is computed with the feature-rich map.

- The left figure shows that the pure NDT causes a longitudinal error in the bridge and is not able to recover.
- The right figure shows that the regularization suppresses the longitudinal error.

<img src="./media/trajectory_without_regularization.png" alt="drawing" width="300"/> <img src="./media/trajectory_with_regularization.png" alt="drawing" width="300"/>

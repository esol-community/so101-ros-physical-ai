# eSOL Extensions

This document describes the functionality added by eSOL to this project.  

## Repository Structure

The following packages were added or modified by eSOL.

```text
so101-ros-physical-ai/
├── so101_bringup/
│   └── launch/              # Added force_feedback.launch.py by eSOL
├── feetech_ros2_driver/     # Added current value notification and torque control by eSOL
├── so101_teleop_force_feedback/  # New package added by eSOL
│   ├── src/
│   ├── config/
│   └── launch/
├── ros2_realtime_support/   # New package added by eSOL
└── so101_msgs/              # New package added by eSOL
    ├── include/  
    ├── src/              
    └── srv/
```

## Added Features

This fork extends the original project with:

- Current-based teleop_force_feedback teleoperation
- Contact detection using follower servo current measurements
- Leader joint lock feedback mechanism
- Servo effort acquisition services for Feetech STS3215 motors
- Optional support for eSOL `ros2_realtime_support`
- Thread affinity and scheduling configuration through YAML

## Related Documentation

For detailed information about the functionality added by eSOL, refer to:  

- [so101_teleop_force_feedback/README.md](./so101_teleop_force_feedback/README.md)
- [README.md](./README.md)

## Acknowledgement

The work was supported by the New Energy and Industrial Technology Development Organization (NEDO), Japan, under commissioned research project JPNP25016.

This acknowledgement applies only to the features added by eSOL.

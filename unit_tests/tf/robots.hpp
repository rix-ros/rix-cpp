#pragma once

static inline const char *SIMPLEBOT_JRDF = R"(
{
  "name": "SimpleBot",
  "links": [
    {
      "name": "base",
      "visuals": [
        {
          "geometry": {
            "type": "box",
            "size": [0.1, 0.1, 0.05]
          },
          "material": {
            "name": "",
            "color": [0.0, 0.15294, 0.29804, 1.0]
          },
          "origin": {
            "xyz": [-0.05, -0.05, 0],
            "rpy": [0, 0, 0]
          }
        }
      ]
    },
    {
      "name": "torso",
      "visuals": [
        {
          "geometry": {
            "type": "box",
            "size": [0.075, 0.075, 0.4]
          },
          "material": {
            "name": "",
            "color": [1.0, 0.79608, 0.01961, 1.0]
          },
          "origin": {
            "xyz": [-0.0375, -0.0375, 0.05],
            "rpy": [0, 0, 0]
          }
        }
      ]
    },
    {
      "name": "upper_arm",
      "visuals": [
        {
          "geometry": {
            "type": "box",
            "size": [0.2, 0.02, 0.02]
          },
          "material": {
            "name": "",
            "color": [0.0, 0.15294, 0.29804, 1.0]
          },
          "origin": {
            "xyz": [0, -0.01, 0],
            "rpy": [0, 0, 0]
          }
        }
      ]
    },
    {
      "name": "forearm",
      "visuals": [
        {
          "geometry": {
            "type": "box",
            "size": [0.1, 0.02, 0.02]
          },
          "material": {
            "name": "",
            "color": [0.0, 0.15294, 0.29804, 1.0]
          },
          "origin": {
            "xyz": [0, -0.01, 0],
            "rpy": [0, 0, 0]
          }
        }
      ]
    },
    {
      "name": "tool",
      "visuals": [
        {
          "geometry": {
            "type": "cylinder",
            "length": 0.025,
            "radius": 0.005
          },
          "material": {
            "name": "",
            "color": [1.0, 0.79608, 0.01961, 1.0]
          },
          "origin": {
            "xyz": [0, 0, 0.01],
            "rpy": [0, 1.5708, 0]
          }
        }
      ]
    },
    {
      "name": "eyeball_left",
      "visuals": [
        {
          "geometry": {
            "type": "sphere",
            "radius": 0.02
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ]
    },
    {
      "name": "eyeball_right",
      "visuals": [
        {
          "geometry": {
            "type": "sphere",
            "radius": 0.02
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ]
    },
    {
      "name": "pupil_left",
      "visuals": [
        {
          "geometry": {
            "type": "sphere",
            "radius": 0.01
          },
          "material": {
            "name": "",
            "color": [0.0, 0.0, 0.0, 1.0]
          },
          "origin": {
            "xyz": [0, -0.015, 0],
            "rpy": [0, 0, 0]
          }
        }
      ]
    },
    {
      "name": "pupil_right",
      "visuals": [
        {
          "geometry": {
            "type": "sphere",
            "radius": 0.01
          },
          "material": {
            "name": "",
            "color": [0.0, 0.0, 0.0, 1.0]
          },
          "origin": {
            "xyz": [0, -0.015, 0],
            "rpy": [0, 0, 0]
          }
        }
      ]
    }
  ],
  "joints": [
    {
      "name": "waist",
      "parent": "base",
      "child": "torso",
      "type": "continuous",
      "axis": [0, 0, 1],
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": 0,
        "upper": 0
      }
    },
    {
      "name": "shoulder",
      "parent": "torso",
      "child": "upper_arm",
      "type": "revolute",
      "axis": [0, 1, 0],
      "origin": {
        "xyz": [0.0375, 0, 0.3],
        "rpy": [0, 0, 0]
      },
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": 0,
        "upper": 1.5708
      }
    },
    {
      "name": "elbow",
      "parent": "upper_arm",
      "child": "forearm",
      "type": "revolute",
      "axis": [0, 1, 0],
      "origin": {
        "xyz": [0.2, 0, 0],
        "rpy": [0, 0, 0]
      },
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": -1.5708,
        "upper": 0
      }
    },
    {
      "name": "wrist",
      "parent": "forearm",
      "child": "tool",
      "type": "prismatic",
      "axis": [0, 1, 0],
      "origin": {
        "xyz": [0.1, 0, 0],
        "rpy": [0, 0, 0]
      },
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": 0,
        "upper": 0.0125
      }
    },
    {
      "name": "eye_socket_left",
      "parent": "torso",
      "child": "eyeball_left",
      "type": "fixed",
      "origin": {
        "xyz": [0.0375, -0.0375, 0.45],
        "rpy": [0, 0, 0]
      }
    },
    {
      "name": "eye_socket_right",
      "parent": "torso",
      "child": "eyeball_right",
      "type": "fixed",
      "origin": {
        "xyz": [-0.0375, -0.0375, 0.45],
        "rpy": [0, 0, 0]
      }
    },
    {
      "name": "eye_lens_left",
      "parent": "eyeball_left",
      "child": "pupil_left",
      "type": "revolute",
      "axis": [0, 0, 1],
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": -1,
        "upper": 1
      }
    },
    {
      "name": "eye_lens_right",
      "parent": "eyeball_right",
      "child": "pupil_right",
      "type": "revolute",
      "axis": [1, 0, 0],
      "limits": {
        "velocity": 100.0,
        "effort": 1000.0,
        "lower": -1,
        "upper": 1
      }
    }
  ]
}
)";

static inline const char *RX200_JRDF = R"(
{
  "name": "ReactorX-200",
  "links": [
    {
      "name": "/base_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-1-Base.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-1-Base.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              -1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            -0.0354389,
            -0.00335861,
            0.0253408
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.766135,
        "inertia": {
          "ixx": 0.003353,
          "ixy": -0.0001246,
          "ixz": 1.106e-05,
          "iyy": 0.001357,
          "iyz": 0.0002103,
          "izz": 0.004111
        }
      }
    },
    {
      "name": "/shoulder_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-2-Shoulder.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0015
            ],
            "rpy": [
              0.0,
              0.0,
              -1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-2-Shoulder.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0015
            ],
            "rpy": [
              0.0,
              0.0,
              -1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            -8.53644e-05,
            1.7369e-05,
            0.0132005
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.257774,
        "inertia": {
          "ixx": 0.0002663,
          "ixy": 9e-10,
          "ixz": 5.11e-08,
          "iyy": 0.0004428,
          "iyz": 4.416e-07,
          "izz": 0.0004711
        }
      }
    },
    {
      "name": "/upper_arm_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-3-UA.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-3-UA.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.0119513,
            -0.000116923,
            0.13943
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.297782,
        "inertia": {
          "ixx": 0.00171,
          "ixy": -9.773e-07,
          "ixz": 2.0936e-06,
          "iyy": 0.001631,
          "iyz": 0.0002132,
          "izz": 0.0001478
        }
      }
    },
    {
      "name": "/forearm_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-4-Forearm.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-4-Forearm.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.114745,
            -9.38376e-05,
            0.0
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.258863,
        "inertia": {
          "ixx": 0.001055,
          "ixy": -1.8286e-06,
          "ixz": 0.0,
          "iyy": 6.421e-05,
          "iyz": 0.0,
          "izz": 0.001076
        }
      }
    },
    {
      "name": "/wrist_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-5-Wrist.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              3.141592653589793,
              -1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-5-Wrist.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              3.141592653589793,
              -1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.04236,
            1.0411e-05,
            -0.010577
          ],
          "rpy": [
            0.0,
            3.141592653589793,
            -1.5707963267948966
          ]
        },
        "mass": 0.084957,
        "inertia": {
          "ixx": 3.082e-05,
          "ixy": 1.91e-08,
          "ixz": 2.3e-09,
          "iyy": 2.822e-05,
          "iyz": 2.5481e-06,
          "izz": 3.152e-05
        }
      }
    },
    {
      "name": "/gripper_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-6-Gripper.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.02,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-6-Gripper.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.02,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.02163,
            0.0,
            0.01141
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.072885,
        "inertia": {
          "ixx": 2.537e-05,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 1.836e-05,
          "iyz": 4.34e-07,
          "izz": 1.674e-05
        }
      }
    },
    {
      "name": "/ee_arm_link",
      "inertial": {
        "origin": {
          "xyz": [
            0,
            0,
            0
          ],
          "rpy": [
            0,
            0,
            0
          ]
        },
        "mass": 0.001,
        "inertia": {
          "ixx": 0.001,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.001,
          "iyz": 0.0,
          "izz": 0.001
        }
      }
    },
    {
      "name": "/gripper_prop_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-7-Gripper-Prop.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.0685,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-7-Gripper-Prop.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.0685,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.000846,
            -1.933e-06,
            4.2e-05
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.00434,
        "inertia": {
          "ixx": 5.923e-07,
          "ixy": 0.0,
          "ixz": 3.195e-07,
          "iyy": 1.1156e-06,
          "iyz": -4e-10,
          "izz": 5.743e-07
        }
      }
    },
    {
      "name": "/gripper_bar_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-8-Gripper-Bar.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.063,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-8-Gripper-Bar.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              -0.063,
              0.0,
              0.0
            ],
            "rpy": [
              0.0,
              0.0,
              1.5707963267948966
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.009687,
            5.66e-07,
            0.004962
          ],
          "rpy": [
            0.0,
            0.0,
            1.5707963267948966
          ]
        },
        "mass": 0.034199,
        "inertia": {
          "ixx": 7.4125e-06,
          "ixy": -8e-10,
          "ixz": -6e-10,
          "iyy": 2.843e-05,
          "iyz": -1.3889e-06,
          "izz": 2.86e-05
        }
      }
    },
    {
      "name": "/fingers_link",
      "inertial": {
        "origin": {
          "xyz": [
            0,
            0,
            0
          ],
          "rpy": [
            0,
            0,
            0
          ]
        },
        "mass": 0.001,
        "inertia": {
          "ixx": 0.001,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.001,
          "iyz": 0.0,
          "izz": 0.001
        }
      }
    },
    {
      "name": "/left_finger_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-9-Finger.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.005,
              0.0
            ],
            "rpy": [
              3.141592653589793,
              3.141592653589793,
              0.0
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-9-Finger.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              0.005,
              0.0
            ],
            "rpy": [
              3.141592653589793,
              3.141592653589793,
              0.0
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.013816,
            0.0,
            0.0
          ],
          "rpy": [
            3.141592653589793,
            3.141592653589793,
            1.5707963267948966
          ]
        },
        "mass": 0.016246,
        "inertia": {
          "ixx": 4.731e-06,
          "ixy": -4.56e-07,
          "ixz": 0.0,
          "iyy": 1.5506e-06,
          "iyz": 0.0,
          "izz": 3.7467e-06
        }
      }
    },
    {
      "name": "/right_finger_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-9-Finger.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              -0.005,
              0.0
            ],
            "rpy": [
              0.0,
              3.141592653589793,
              0.0
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "rx200/RXA-200-M-9-Finger.stl",
            "scale": [
              0.001,
              0.001,
              0.001
            ]
          },
          "origin": {
            "xyz": [
              0.0,
              -0.005,
              0.0
            ],
            "rpy": [
              0.0,
              3.141592653589793,
              0.0
            ]
          },
          "material": {
            "name": "interbotix_black"
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.013816,
            0.0,
            0.0
          ],
          "rpy": [
            0.0,
            3.141592653589793,
            1.5707963267948966
          ]
        },
        "mass": 0.016246,
        "inertia": {
          "ixx": 4.731e-06,
          "ixy": 4.56e-07,
          "ixz": 0.0,
          "iyy": 1.5506e-06,
          "iyz": 0.0,
          "izz": 3.7467e-06
        }
      }
    },
    {
      "name": "/ee_gripper_link",
      "inertial": {
        "origin": {
          "xyz": [
            0,
            0,
            0
          ],
          "rpy": [
            0,
            0,
            0
          ]
        },
        "mass": 0.001,
        "inertia": {
          "ixx": 0.001,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.001,
          "iyz": 0.0,
          "izz": 0.001
        }
      }
    }
  ],
  "joints": [
    {
      "name": "waist",
      "type": "revolute",
      "parent": "/base_link",
      "child": "/shoulder_link",
      "origin": {
        "xyz": [
          0.0,
          0.0,
          0.065
        ],
        "rpy": [
          0.0,
          0.0,
          3.141592653589793
        ]
      },
      "axis": [
        0.0,
        0.0,
        1.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 3.141592653589793,
        "lower": -3.141592653589793,
        "upper": 3.141592653589793
      }
    },
    {
      "name": "shoulder",
      "type": "revolute",
      "parent": "/shoulder_link",
      "child": "/upper_arm_link",
      "origin": {
        "xyz": [
          0.0,
          0.0,
          0.03891
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        0.0,
        -1.0,
        0.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 1.0,
        "lower": -1.8675022996339325,
        "upper": 1.9373154697137058
      }
    },
    {
      "name": "elbow",
      "type": "revolute",
      "parent": "/upper_arm_link",
      "child": "/forearm_link",
      "origin": {
        "xyz": [
          0.05,
          0.0,
          0.2
        ],
        "rpy": [
          3.141592653589793,
          0.0,
          0.0
        ]
      },
      "axis": [
        0.0,
        1.0,
        0.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 3.141592653589793,
        "lower": -1.6231562043547265,
        "upper": 1.8849555921538759
      }
    },
    {
      "name": "wrist_angle",
      "type": "revolute",
      "parent": "/forearm_link",
      "child": "/wrist_link",
      "origin": {
        "xyz": [
          0.2,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        0.0,
        1.0,
        0.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 3.141592653589793,
        "lower": -2.1467549799530254,
        "upper": 1.7453292519943295
      }
    },
    {
      "name": "wrist_rotate",
      "type": "revolute",
      "parent": "/wrist_link",
      "child": "/gripper_link",
      "origin": {
        "xyz": [
          0.065,
          0.0,
          0.0
        ],
        "rpy": [
          -3.141592653589793,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 3.141592653589793,
        "lower": -3.141592653589793,
        "upper": 3.141592653589793
      }
    },
    {
      "name": "ee_arm",
      "type": "fixed",
      "parent": "/gripper_link",
      "child": "/ee_arm_link",
      "origin": {
        "xyz": [
          0.043,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ]
    },
    {
      "name": "gripper",
      "type": "continuous",
      "parent": "/ee_arm_link",
      "child": "/gripper_prop_link",
      "origin": {
        "xyz": [
          0.0055,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ],
      "limits": {
        "effort": 100.0,
        "velocity": 3.141592653589793,
        "lower": 0,
        "upper": 0
      }
    },
    {
      "name": "gripper_bar",
      "type": "fixed",
      "parent": "/ee_arm_link",
      "child": "/gripper_bar_link",
      "origin": {
        "xyz": [
          0.0,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ]
    },
    {
      "name": "ee_bar",
      "type": "fixed",
      "parent": "/gripper_bar_link",
      "child": "/fingers_link",
      "origin": {
        "xyz": [
          0.023,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ]
    },
    {
      "name": "left_finger",
      "type": "prismatic",
      "parent": "/fingers_link",
      "child": "/left_finger_link",
      "origin": {
        "xyz": [
          0.0,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        0.0,
        1.0,
        0.0
      ],
      "limits": {
        "effort": 10.0,
        "velocity": 1.0,
        "lower": 0.015,
        "upper": 0.037
      },
      "mimic": {
        "multiplier": 0.014,
        "offset": 0.014,
        "name": "gripper"
      }
    },
    {
      "name": "right_finger",
      "type": "prismatic",
      "parent": "/fingers_link",
      "child": "/right_finger_link",
      "origin": {
        "xyz": [
          0.0,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        0.0,
        1.0,
        0.0
      ],
      "limits": {
        "effort": 10.0,
        "velocity": 1.0,
        "lower": -0.037,
        "upper": -0.015
      },
      "mimic": {
        "multiplier": -1.0,
        "offset": 0.0,
        "name": "left_finger"
      }
    },
    {
      "name": "ee_gripper",
      "type": "fixed",
      "parent": "/fingers_link",
      "child": "/ee_gripper_link",
      "origin": {
        "xyz": [
          0.027575,
          0.0,
          0.0
        ],
        "rpy": [
          0.0,
          0.0,
          0.0
        ]
      },
      "axis": [
        1.0,
        0.0,
        0.0
      ]
    }
  ],
  "materials": [
    {
      "name": "interbotix_black",
      "filename": "rx200/interbotix_black.png"
    }
  ]
}
)";

static inline const char *FETCH_JRDF = R"(
{
  "name": "fetch",
  "links": [
    {
      "name": "base_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/base_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.356, 0.361, 0.376, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/base_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.0036, 0.0, 0.0014],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 70.1294,
        "inertia": {
          "ixx": 1.225,
          "ixy": 0.0099,
          "ixz": 0.0062,
          "iyy": 1.2853,
          "iyz": -0.0034,
          "izz": 0.987
        }
      }
    },
    {
      "name": "torso_lift_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/torso_lift_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/torso_lift_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.0013, -0.0009, 0.2935],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 10.7796,
        "inertia": {
          "ixx": 0.3354,
          "ixy": 0.0,
          "ixz": -0.0162,
          "iyy": 0.3354,
          "iyz": -0.0006,
          "izz": 0.0954
        }
      }
    },
    {
      "name": "shoulder_pan_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/shoulder_pan_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/shoulder_pan_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.0927, -0.0056, 0.0564],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 2.5587,
        "inertia": {
          "ixx": 0.0043,
          "ixy": -0.0001,
          "ixz": 0.001,
          "iyy": 0.0087,
          "iyz": -0.0001,
          "izz": 0.0087
        }
      }
    },
    {
      "name": "shoulder_lift_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/shoulder_lift_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.086, 0.506, 0.767, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/shoulder_lift_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.1432, 0.0072, -0.0001],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 2.6615,
        "inertia": {
          "ixx": 0.0028,
          "ixy": -0.0021,
          "ixz": -0.0,
          "iyy": 0.0111,
          "iyz": -0.0,
          "izz": 0.0112
        }
      }
    },
    {
      "name": "upperarm_roll_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/upperarm_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/upperarm_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.1165, 0.0014, 0.0],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 2.3311,
        "inertia": {
          "ixx": 0.0019,
          "ixy": -0.0001,
          "ixz": 0.0,
          "iyy": 0.0045,
          "iyz": 0.0,
          "izz": 0.0047
        }
      }
    },
    {
      "name": "elbow_flex_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/elbow_flex_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.086, 0.506, 0.767, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/elbow_flex_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.1279, 0.0073, 0.0],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 2.1299,
        "inertia": {
          "ixx": 0.0024,
          "ixy": -0.0016,
          "ixz": 0.0,
          "iyy": 0.0082,
          "iyz": -0.0,
          "izz": 0.0084
        }
      }
    },
    {
      "name": "forearm_roll_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/forearm_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/forearm_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.1097, -0.0266, 0.0],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 1.6563,
        "inertia": {
          "ixx": 0.0016,
          "ixy": -0.0003,
          "ixz": 0.0,
          "iyy": 0.003,
          "iyz": -0.0,
          "izz": 0.0035
        }
      }
    },
    {
      "name": "wrist_flex_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/wrist_flex_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.086, 0.506, 0.767, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/wrist_flex_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.0882, 0.0009, -0.0001],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 1.725,
        "inertia": {
          "ixx": 0.0018,
          "ixy": -0.0001,
          "ixz": -0.0,
          "iyy": 0.0042,
          "iyz": 0.0,
          "izz": 0.0042
        }
      }
    },
    {
      "name": "wrist_roll_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/wrist_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [1.0, 1.0, 1.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/wrist_roll_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.0095, 0.0004, -0.0002],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.1354,
        "inertia": {
          "ixx": 0.0001,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.0001,
          "iyz": 0.0,
          "izz": 0.0001
        }
      }
    },
    {
      "name": "gripper_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/gripper_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/gripper_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.09, -0.0001, -0.0017],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 1.5175,
        "inertia": {
          "ixx": 0.0013,
          "ixy": -0.0,
          "ixz": 0.0,
          "iyy": 0.0019,
          "iyz": -0.0,
          "izz": 0.0024
        }
      }
    },
    {
      "name": "r_gripper_finger_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/r_gripper_finger_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.101425, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.356, 0.361, 0.376, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/r_gripper_finger_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.101425, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.01, 0.0, 0.0],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.0798,
        "inertia": {
          "ixx": 0.002,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.0,
          "iyz": 0.0,
          "izz": 0.0
        }
      }
    },
    {
      "name": "l_gripper_finger_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/l_gripper_finger_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, -0.101425, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.356, 0.361, 0.376, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/l_gripper_finger_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, -0.101425, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.01, 0.0, 0.0],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.0798,
        "inertia": {
          "ixx": 0.002,
          "ixy": 0.0,
          "ixz": 0.0,
          "iyy": 0.0,
          "iyz": 0.0,
          "izz": 0.0
        }
      }
    },
    {
      "name": "bellows_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/bellows_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.0, 0.0, 0.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/bellows_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.0191320000286228, -1.67219873481315e-16, -0.134861625443387
          ],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.169374038216602,
        "inertia": {
          "ixx": 0.00331159128762117,
          "ixy": -8.9230964321455e-18,
          "ixz": -5.38622201018293e-8,
          "iyy": 0.00174447292786627,
          "iyz": -8.06964979976371e-17,
          "izz": 0.00169417568121457
        }
      }
    },
    {
      "name": "bellows_link2",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/bellows_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.0, 0.0, 0.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/bellows_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [
            0.0191320000286228, -1.67219873481315e-16, -0.134861625443387
          ],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.169374038216602,
        "inertia": {
          "ixx": 0.00331159128762117,
          "ixy": -8.9230964321455e-18,
          "ixz": -5.38622201018293e-8,
          "iyy": 0.00174447292786627,
          "iyz": -8.06964979976371e-17,
          "izz": 0.00169417568121457
        }
      }
    },
    {
      "name": "estop_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/estop_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.8, 0.0, 0.0, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/estop_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [0.002434512737072, -0.00330608315239905, 0.00665139196650039],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.00196130439134723,
        "inertia": {
          "ixx": 3.02810026604417e-7,
          "ixy": -1.5862023118056e-8,
          "ixz": 3.16561396557437e-8,
          "iyy": 2.93322917127605e-7,
          "iyz": -4.28833522751273e-8,
          "izz": 2.28513272627183e-7
        }
      }
    },
    {
      "name": "laser_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/laser_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [
              0.792156862745098, 0.819607843137255, 0.933333333333333, 1.0
            ]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/laser_link.STL",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.0306228970175641, 0.0007091682908278, 0.0551974119471302],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 0.00833634573995571,
        "inertia": {
          "ixx": 1.01866461240801e-6,
          "ixy": -5.88447626567756e-8,
          "ixz": 7.83857244757914e-8,
          "iyy": 5.10039589974707e-7,
          "iyz": -7.12664289617235e-9,
          "izz": 1.28270671527309e-6
        }
      }
    },
    {
      "name": "torso_fixed_link",
      "visuals": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/torso_fixed_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          },
          "material": {
            "name": "",
            "color": [0.086, 0.506, 0.767, 1.0]
          }
        }
      ],
      "collisions": [
        {
          "geometry": {
            "type": "mesh",
            "filename": "fetch/torso_fixed_link.obj",
            "scale": [1, 1, 1]
          },
          "origin": {
            "xyz": [0.0, 0.0, 0.0],
            "rpy": [0.0, 0.0, 0.0]
          }
        }
      ],
      "inertial": {
        "origin": {
          "xyz": [-0.0722, 0.0057, 0.2656],
          "rpy": [0.0, 0.0, 0.0]
        },
        "mass": 13.2775,
        "inertia": {
          "ixx": 0.3861,
          "ixy": 0.0015,
          "ixz": -0.0102,
          "iyy": 0.3388,
          "iyz": 0.0117,
          "izz": 0.1018
        }
      }
    }
  ],
  "joints": [
    {
      "name": "torso_lift_joint",
      "type": "prismatic",
      "parent": "base_link",
      "child": "torso_lift_link",
      "origin": {
        "xyz": [-0.086875, 0.0, 0.37743],
        "rpy": [-6.123e-17, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, 1.0],
      "limits": {
        "effort": 450.0,
        "velocity": 0.1,
        "lower": 0.0,
        "upper": 0.38615
      },
      "dynamics": {
        "damping": 100.0
      }
    },
    {
      "name": "shoulder_pan_joint",
      "type": "revolute",
      "parent": "torso_lift_link",
      "child": "shoulder_pan_link",
      "origin": {
        "xyz": [0.119525, 0.0, 0.34858],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, 1.0],
      "limits": {
        "effort": 33.82,
        "velocity": 1.256,
        "lower": -1.6056,
        "upper": 1.6056
      },
      "dynamics": {
        "damping": 1.0
      }
    },
    {
      "name": "shoulder_lift_joint",
      "type": "revolute",
      "parent": "shoulder_pan_link",
      "child": "shoulder_lift_link",
      "origin": {
        "xyz": [0.117, 0.0, 0.0599999999999999],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0],
      "limits": {
        "effort": 131.76,
        "velocity": 1.454,
        "lower": -1.221,
        "upper": 1.518
      },
      "dynamics": {
        "damping": 1.0
      }
    },
    {
      "name": "upperarm_roll_joint",
      "type": "continuous",
      "parent": "shoulder_lift_link",
      "child": "upperarm_roll_link",
      "origin": {
        "xyz": [0.219, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [1.0, 0.0, 0.0],
      "limits": {
        "effort": 76.94,
        "velocity": 1.571,
        "lower": 0,
        "upper": 0
      },
      "dynamics": {
        "damping": 5.0
      }
    },
    {
      "name": "elbow_flex_joint",
      "type": "revolute",
      "parent": "upperarm_roll_link",
      "child": "elbow_flex_link",
      "origin": {
        "xyz": [0.133, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0],
      "limits": {
        "effort": 66.18,
        "velocity": 1.521,
        "lower": -2.251,
        "upper": 2.251
      },
      "dynamics": {
        "damping": 1.0
      }
    },
    {
      "name": "forearm_roll_joint",
      "type": "continuous",
      "parent": "elbow_flex_link",
      "child": "forearm_roll_link",
      "origin": {
        "xyz": [0.197, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [1.0, 0.0, 0.0],
      "limits": {
        "effort": 29.35,
        "velocity": 1.571,
        "lower": 0,
        "upper": 0
      },
      "dynamics": {
        "damping": 5.0
      }
    },
    {
      "name": "wrist_flex_joint",
      "type": "revolute",
      "parent": "forearm_roll_link",
      "child": "wrist_flex_link",
      "origin": {
        "xyz": [0.1245, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0],
      "limits": {
        "effort": 25.7,
        "velocity": 2.268,
        "lower": -2.16,
        "upper": 2.16
      },
      "dynamics": {
        "damping": 1.0
      }
    },
    {
      "name": "wrist_roll_joint",
      "type": "continuous",
      "parent": "wrist_flex_link",
      "child": "wrist_roll_link",
      "origin": {
        "xyz": [0.1385, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [1.0, 0.0, 0.0],
      "limits": {
        "effort": 7.36,
        "velocity": 2.268,
        "lower": 0,
        "upper": 0
      },
      "dynamics": {
        "damping": 5.0
      }
    },
    {
      "name": "gripper_axis",
      "type": "fixed",
      "parent": "wrist_roll_link",
      "child": "gripper_link",
      "origin": {
        "xyz": [0.16645, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0]
    },
    {
      "name": "r_gripper_finger_joint",
      "type": "fixed",
      "parent": "gripper_link",
      "child": "r_gripper_finger_link",
      "origin": {
        "xyz": [0.0, 0.015425, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0]
    },
    {
      "name": "l_gripper_finger_joint",
      "type": "fixed",
      "parent": "gripper_link",
      "child": "l_gripper_finger_link",
      "origin": {
        "xyz": [0.0, -0.015425, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, -1.0, 0.0]
    },
    {
      "name": "bellows_joint",
      "type": "fixed",
      "parent": "torso_lift_link",
      "child": "bellows_link",
      "origin": {
        "xyz": [0.0, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, -1.0]
    },
    {
      "name": "bellows_joint2",
      "type": "fixed",
      "parent": "torso_lift_link",
      "child": "bellows_link2",
      "origin": {
        "xyz": [0.0, 0.0, 0.0],
        "rpy": [0.0, 0.0, 0.0]
      }
    },
    {
      "name": "estop_joint",
      "type": "fixed",
      "parent": "base_link",
      "child": "estop_link",
      "origin": {
        "xyz": [-0.12465, 0.23892, 0.31127],
        "rpy": [1.5708, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, 0.0]
    },
    {
      "name": "laser_joint",
      "type": "fixed",
      "parent": "base_link",
      "child": "laser_link",
      "origin": {
        "xyz": [0.235, 0.0, 0.2878],
        "rpy": [3.14159265359, 0.0, 0.0]
      },
      "axis": [0.0, 0.0, 0.0]
    },
    {
      "name": "torso_fixed_joint",
      "type": "fixed",
      "parent": "base_link",
      "child": "torso_fixed_link",
      "origin": {
        "xyz": [-0.086875, 0.0, 0.377425],
        "rpy": [-6.12303176911189e-17, 0.0, 0.0]
      },
      "axis": [0.0, 1.0, 0.0]
    }
  ]
}
)";
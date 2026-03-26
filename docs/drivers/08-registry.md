Driver Registry
==============

1. Overview
-----------

The driver registry provides a centralized mechanism for driver
initialization and tracking.

2. Structure
------------

  struct driver {
      const char *name;
      void (*init)(void);
  };

3. API
-----

  void registry_add_driver(struct driver *drv)
      - Register a driver for initialization

  void registry_init_all(void)
      - Initialize all registered drivers

4. Usage
-------

Drivers call registry_add_driver during their initialization to
register themselves with the system.

5. Initialization Order
-----------------------

The registry ensures drivers are initialized in the correct order
based on dependencies.

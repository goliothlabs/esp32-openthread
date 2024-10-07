..
   Copyright (c) 2024 Golioth, Inc.
   SPDX-License-Identifier: Apache-2.0

Golioth ESP32-C6 OpenThread Example
###################################

Overview
********
This sample application demonstrates how to connect an ESP32-C6 to Golioth
over OpenThread communication protocol. The application implements Golioth's
Application Services and Device Management capabilities.

Requirements
************

- Golioth device PSK credentials
- A running Thread Border Router with NAT64 translation (we will be using the
  commercially available off-the-shelf `GL-S200 Thread Border Router`_)
- Thread Network Name, Network Key and PAN ID
- ESP-IDF Release v5.3

Local set up
************

.. code-block:: shell

   git clone --recursive git@github.com:goliothlabs/esp32-openthread.git

Or, if you've already cloned but forgot the ``--recursive``, you can update and
initialize submodules with this command:

.. code-block:: shell

   cd esp32-openthread/golioth-firmware-sdk
   git submodule update --init --recursive

Building and Running
********************

Configure Thread Network
========================

Configure the following options in ``sdkconfig.defaults`` file:

   * OPENTHREAD_NETWORK_NAME       - Name of your Thread network
   * OPENTHREAD_NETWORK_CHANNEL    - Thread Network Channel
   * OPENTHREAD_NETWORK_PANID      - Thread network PAN ID
   * OPENTHREAD_NETWORK_MASTERKEY  - Network Key of your Thread network

.. pull-quote::
   [!IMPORTANT]

   Make sure the Thread Network Name, Thread Network Key, Thread Channel and
   PAN ID match your Border Router configuration.

Command-line
============

Setup the environment, this step assumes you've installed esp-idf v5.3
to ``~/esp/esp-idf``. If you haven't, follow the initial steps in
examples/esp_idf/README.md

.. code-block:: shell

   source ~/esp/esp-idf/export.sh

From the ``esp32-openthread`` directory, set target chip to ESP32-C6:

.. code-block:: shell

   idf.py set-target esp32c6


``build``, ``flash`` and ``monitor`` the console output.

.. code-block:: shell

   idf.py build
   idf.py flash
   idf.py monitor


Shell Credentials
=================

.. code-block:: shell

   settings set golioth/psk-id "YourGoliothDevicePSK-ID"
   settings set golioth/psk "YourGoliothDevicePSK"

Type `reset` to restart the app with the new credentials.

.. pull-quote::
   [!IMPORTANT]

   Wait until the device is connected to the Thread network and receives the NAT64
   prefix required to synthesize Golioth System Server IPv6 address.

Add Pipeline to Golioth
***********************

Head over to `Golioth Console`_ to add `Pipelines`_ to route stream data. 
`Pipelines`_ give you the flexibility to change your data routing without 
requiring updated device firmware.

Whenever sending stream data, you must enable a pipeline in your Golioth project to configure how
that data is handled. Add the contents of ``pipelines/cbor-to-lightdb.yml`` and 
``pipelines/json-to-lightdb.yml`` create two new pipelines as follows (note that ``cbor-to-lightdb.yml``
is the default pipeline for new projects and may already be present):

   1. Navigate to your project on the Golioth web console.
   2. Select ``Pipelines`` from the left sidebar and click the ``Create`` button.
   3. Give your new pipeline a name and paste the pipeline configuration into the editor.
   4. Click the toggle in the bottom right to enable the pipeline and then click ``Create``.

All data streamed to Golioth in CBOR or JSON format will now be routed to LightDB Stream and may be viewed
using the web console. You may change this behavior at any time without updating firmware simply by
editing the pipeline entry.

Golioth Features
****************

This firmware implements the following features from the Golioth Zephyr SDK:

- `Device Settings Service <https://docs.golioth.io/firmware/golioth-firmware-sdk/device-settings-service/>`_
- `LightDB State Client <https://docs.golioth.io/firmware/golioth-firmware-sdk/light-db-state/>`_
- `LightDB Stream Client <https://docs.golioth.io/firmware/golioth-firmware-sdk/stream-client/>`_
- `Logging Client <https://docs.golioth.io/device-management/logging/>`_
- `Over-the-Air (OTA) Firmware Upgrade <https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade>`_
- `Remote Procedure Call (RPC) <https://docs.golioth.io/firmware/golioth-firmware-sdk/remote-procedure-call>`_


Settings Service
================

The following settings should be set in the Device Settings menu of the
`Golioth Console`_.

``LOOP_DELAY_S``
   Adjusts the delay between sensor readings. Set to an integer value (seconds).

   Default value is ``60`` seconds.

Remote Procedure Call (RPC) Service
===================================

The following RPCs can be initiated in the Remote Procedure Call menu of the
`Golioth Console`_.

``reboot``
   Reboot the system.

``set_log_level``
   Set the log level.

   The method takes a single parameter which can be one of the following integer
   values:

   * ``0``: ``ESP_LOGE``        - Error (lowest)
   * ``1``: ``ESP_LOGW``        - Warning
   * ``2``: ``ESP_LOGI``        - Info
   * ``3``: ``ESP_LOGD``        - Debug
   * ``4``: ``ESP_LOGV``        - Verbose (highest)

LightDB State and LightDB Stream data
=====================================

Time-Series Data (LightDB Stream)
---------------------------------

An up-counting counter is periodically sent to the ``cbor/counter`` and ``json/counter``
endpoints of the LightDB Stream service to simulate sensor data.
Sending period is based on the ``LOOP_DELAY_S`` settings.

Stateful Data (LightDB State)
-----------------------------

The concept of Digital Twin is demonstrated with the LightDB State
``example_int0`` and ``example_int1`` variables that are members of the ``desired``
and ``state`` endpoints.

* ``desired`` values may be changed from the cloud side. The device will recognize
  these, validate them for [0..65535] bounding, and then reset these endpoints
  to ``-1``

* ``state`` values will be updated by the device whenever a valid value is
  received from the ``desired`` endpoints. The cloud may read the ``state``
  endpoints to determine device status, but only the device should ever write to
  the ``state`` endpoints.


OTA Firmware Update
*******************

This sample application demonstrates how to perform a Firmware
Update using Golioth's Over-the-Air (OTA) update service. This is
a two step process:

- Build initial firmware and flash it to the device.
- Build new firmware to use as the upgrade and upload it to Golioth.

Edit the ``app_main.c`` file and update the firmware version number:

.. code-block:: shell

   static const char *_current_version = "1.0.1";

Build the firmware update but do not flash it to the device. The binary update file
``esp_c6_openthread.bin`` located in the ``build`` directory needs to be uploaded to 
Golioth for the OTA update as an Artifact.
After uploading the artifact, you can create a new release using that artifact.

Visit `the Golioth Docs OTA Firmware Upgrade page`_ for more info and follow the
steps from `Zephyr Firmware Update Example Walkthrough`_.

.. pull-quote::
   [!IMPORTANT]

   Update Example Walkthrough is based on Zephyr RTOS and not ESP-IDF.

.. _Golioth Console: https://console.golioth.io
.. _Pipelines: https://docs.golioth.io/data-routing
.. _the Golioth Docs OTA Firmware Upgrade page: https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade
.. _Zephyr Firmware Update Example Walkthrough: https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/fw-update-zephyr#4-upload-new-firmware-to-the-golioth-console
.. _GL-S200 Thread Border Router: https://www.gl-inet.com/products/gl-s200/

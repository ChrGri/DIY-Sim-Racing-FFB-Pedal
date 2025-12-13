# Daily Build Instructions

## Steps

1. Open the project in VS Code and build the firmware with PlatformIO CLI:
    ```sh
    pio run
    ```

2. After building, execute the corresponding batch file to copy the firmware to the OTA DailyBuild folder:
    - For Control Board firmware:
      ```sh
      DIY-Sim-Racing-FFB-Pedal\OTA\DailyBuild\copy_to_OTA_folder_ControlBoard.bat
      ```
    - For Bridge firmware:
      ```sh
      DIY-Sim-Racing-FFB-Pedal\OTA\DailyBuild\copy_to_folder_bridge.bat
      ```

3. Commit and push your changes:
    ```sh
    git add .
    git commit -m "Daily build update"
    git push
    ```


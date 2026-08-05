# OTA Release Guideline

Follow these steps to successfully publish a new release:

1. **Compile `esp32` Environments**
   Compile all environments (`envs`) located under the `esp32` folder.

2. **Compile `esp32_master` Environments**
   Compile all environments (`envs`) located under the `esp32_master` folder.

3. **Build Simhub Plugin (Release Mode)**
   Open the `DiyFfbPedal.sln` solution inside the `SimhubPlugin` folder and compile it using the **Release** configuration.

4. **Run Build Scripts**
   Navigate to the `OTA/ReleaseBuild/` directory and execute both of the following batch scripts:
   - `copy_to_folder_bridge.bat`
   - `copy_to_OTA_folder_ControlBoard.bat`

5. **Copy the Plugin DLL**
   Copy the newly built `DiyFfbPedal.dll` from the `SimhubPlugin/bin/` directory into both of the following destination folders:
   - `OTA/ReleaseBuild/Plugin`
   - `OTA/DevBuild/plugin`

6. **Update Version and Changelog**
   Edit `version_control.json` in its raw format and update the details:
   - Change the version number to the format `YY.WW.DD`, where:
     - `YY` = Year
     - `WW` = Week of the year
     - `DD` = Day of the week (Note: Sunday is 1st)
   - Update the `changelog` with the release notes.

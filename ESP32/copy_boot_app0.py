Import("env")
import os
import shutil

def copy_boot_app(source, target, env):
    # Find the framework directory where PlatformIO stores the core files
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    if not framework_dir:
        print("Warning: framework-arduinoespressif32 not found!")
        return

    # Path to the source boot_app0.bin
    boot_app_src = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")
    
    # Path to the current build directory (.pio/build/your_env/)
    build_dir = env.subst("$BUILD_DIR")
    boot_app_dest = os.path.join(build_dir, "boot_app0.bin")

    # Perform the copy
    if os.path.exists(boot_app_src):
        print(f"\n--- Copying boot_app0.bin to build directory ---")
        print(f"Dest: {boot_app_dest}\n")
        shutil.copy(boot_app_src, boot_app_dest)
    else:
        print(f"Error: boot_app0.bin not found at {boot_app_src}")

# Hook this function to run AFTER the firmware.bin is generated
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_boot_app)
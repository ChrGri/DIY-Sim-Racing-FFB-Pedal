Import("env")
import datetime
import re
import os

def update_version(*args, **kwargs):
    # Get the project path
    project_dir = env.subst("$PROJECT_DIR")
    header_path = os.path.join(project_dir, "include", "Version_Board.h")
    
    if not os.path.exists(header_path):
        print(f"Version_Board.h not found at {header_path}")
        return

    now = datetime.datetime.now()
    year, week, weekday = now.isocalendar()
    year_str = str(year)[-2:]
    version_str = f'"{year_str}.{week:02d}.{weekday:02d}"'
    
    with open(header_path, "r", encoding="utf-8") as f:
        content = f.read()
        
    # Find and replace the version string, keeping the rest of the file intact
    new_content = re.sub(
        r'(const\s+char\s+\*DAP_FIRMWARE_VERSION\s*=\s*)".*?"',
        fr'\1{version_str}',
        content
    )
    
    if new_content != content:
        with open(header_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"Updated firmware version to {version_str} in {header_path}")

# Run the update
update_version()

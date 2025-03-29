1) Copy hidapi.dll + lib + pdb to C:\Windows\System32
2) run the test_hid_latency.py python script once. It will 
3) It will print all connected HID devices
4) Search manually for the VID and PID strings and adapt the python script 
5) Rerun the python script and modulate the pedal 
6) A new gamepad_data.txt will be generated containing the gamepad data
7) Goto https://chart-studio.plotly.com/create/#/ and draw the plot
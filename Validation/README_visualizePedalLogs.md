# Record pedal state logs
1) Open the Simhub plugin and connect to the pedal.
2) Activate the "Capture pedal trace" switch <br> <img width="200" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/6a07a5b5-5135-40d7-99cf-33c8f27d77e9"> <br>
3) The pedal state record can be found in you Simhub installation directory, e.g. "C:\Program Files (x86)\SimHub\PluginsData\Common\DiyFfbPedal\log"
<br> <img width="500" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/34a7df8e-825f-43bd-8bda-93cd2ffd76b1">. <br>
4) Now perform whatever test you want.
5) Deactivate "Capture pedal trace" switch again to finalize the capture.

# Visualize the pedal logs
## Option 1: Via Google Colab:
1) Open this [link](https://colab.research.google.com/github/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/Validation/VisualizePedalLog.ipynb)
2) Upload the pedal log file, see <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/e4215739-4a6f-40ac-8531-cf2c7b5e2120"> <br>
3) Run the code <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/0183959a-20a7-4bb5-920b-bb88a95e3cb2"> <br>
4) Inspect the generated graphs, e.g. <br> <img width="980" alt="Bildschirmfoto 2024-11-01 um 20 48 59" src="https://github.com/user-attachments/assets/d8d3c60a-5d7f-44c2-a404-40fc3253edc1">


## Option 2: Via python fiddle
1) Open [jupyter online](https://python-fiddle.com/)
2) Upload the plotting code by opening "upload code context menu" <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/878bfa80-b024-4f1e-bb99-c0760aeb9418"> <br>
and inserting the [jupyther notebook](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/Validation/VisualizePedalLog.ipynb) link <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/c6f1538d-eec2-43c0-b3dd-773b0bd40678">  <br>

3) Upload the pedal log file (e.g. "C:\Program Files (x86)\SimHub\PluginsData\Common\DiyFfbPedalStateLog_1.txt") <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 44 36" src="https://github.com/user-attachments/assets/2b86cc91-302d-4b39-808c-e154702d0e92">

4) Run the script <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 46 36" src="https://github.com/user-attachments/assets/76f00347-1ef6-413e-806a-3c908aa736a3">

5) Inspect the generated graphs, e.g. <br> <img width="980" alt="Bildschirmfoto 2024-11-01 um 20 48 59" src="https://github.com/user-attachments/assets/d8d3c60a-5d7f-44c2-a404-40fc3253edc1">

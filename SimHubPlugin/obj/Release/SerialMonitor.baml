   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null7 clr-namespace:User.PluginSdkDemoUser.PluginSdkDemo  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles M7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.PluginsSimHub.Plugins.UI U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls -    'User.PluginSdkDemo.SerialMonitor_Window   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  
   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  
   @d2http://schemas.microsoft.com/expression/blend/2008  
   2local clr-namespace:User.PluginSdkDemo  
   Nstyles;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.Plugins  
   Fui7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.Plugins  
   Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  
   -     � Title$  Pedal Log Monitor��$	��815��$	��380�� � WindowStyle$ 
ToolWindow=�$	��380��$	��500����Z� .��� ���	#CC252525.+��� ���	#7F4E4E4E �� CornerRadius$ 5,5,5,5��$��0,0,0,0q�$	��Top=�$��1��$��1��$��0,2,0,0q�._��� -    ��Name$ StackPanel_UI��$��Center=�$	��Top=�.+��� $��Center=�$��25��$	��Top=� �� Orientation$ 
Horizontal=�$	��305��$��RightToLeft=�.+�+  %SimHub.Plugins.Styles.SHButtonPrimary  -     Name$ btn_Serial_Window_clear��$��30��$��Clear��$��20��$��0,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20���� $��20��$	��160��$ 
Horizontal=�$
��Left=�$��Center=�$��10,0,0,0q�.+�*  $SimHub.Plugins.Styles.SHToggleButton  -     Name$4 .Checkbox_Serial_Window_Auto_Remove_Serial_Text��$�� �� d� 	IsChecked$ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)� � ToolTip$( "Dump pedal realtime status to file��	 J� RenderTransformOrigin$	 0.5,0.5&�$��	-10,0,0,0q��� $��
Auto clear��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� -   
 ��Name$
 TextBox_SerialMonitor��$��Visible=�$
��Auto=�$
��Left=�$��2��$��2��$��4�� �� TextWrapping$
 Wrap=�$�� ��$	��Top=�4  .System.Windows.Automation.AutomationProperties   Name$ TextBox3�����	#00252525���	#00CCCCCC$��0,0,0,0q�$	��768��$	��370��$��5,0,0,0q�
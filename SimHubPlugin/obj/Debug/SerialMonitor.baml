   M S B A M L   `   `   `  ����I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null7 clr-namespace:User.PluginSdkDemoUser.PluginSdkDemo  5   	   P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles 5   	   M7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.PluginsSimHub.Plugins.UI 5   	   U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 5	   	   -    'User.PluginSdkDemo.SerialMonitor_Window   5   	   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 5   	   R
 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  
   5   	   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  
   5   	   @d2http://schemas.microsoft.com/expression/blend/2008  
   5   	   2local clr-namespace:User.PluginSdkDemo  
   5   	   Nstyles;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.Plugins  
   5   	   Fui7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.Plugins  
   5   	   Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  
   5	   	   -   5          � Title$  Pedal Log Monitor��5
   	   $	��815��6#   $	��380��60    � WindowStyle$ 
ToolWindow=�6R   $	��380��6�   $	��500��6�   ��6<   Z� .��5      � ���	#CC252525.+�5   
   �� ���	#7F4E4E4E �� CornerRadius$ 5,5,5,5��6(   $��0,0,0,0q�6?   $	��Top=�6Y   $��1��6q   $��1��6�   $��0,2,0,0q�6�   ._�5      �� -   5         ��Name$ StackPanel_UI��5      $��Center=�60   $	��Top=�6M   .+�5      �� $��Center=�6   $��25��6:   $	��Top=�6F    �� Orientation$ 
Horizontal=�6^   $	��305��6w   $��RightToLeft=�6�   .+�5      +  %SimHub.Plugins.Styles.SHButtonPrimary  -   5          Name$ btn_Serial_Window_clear��5   -   $��30��6t   $��Clear��6   $��20��6�   $��0,0,0,0q�6�   $��Center=�6�   $��Center=�6�   $��0,0,0,0q�6�   $��Center=�6�   $��Arial)�6  $��10(�6+  $��20��69  �� 5      $��20��6!   $	��160��6-   $ 
Horizontal=�69   $
��Left=�6R   $��Center=�6m   $��10,0,0,0q�6�   .+�5      *  $SimHub.Plugins.Styles.SHToggleButton  -   5          Name$4 .Checkbox_Serial_Window_Auto_Remove_Serial_Text��5   0   $�� ��6h    d� 	IsChecked$ FalseY�6�   $��10(�6  $��35��6  $��15��6$  $
��Left=�60  $��Arial)�6K   � ToolTip$( "Dump pedal realtime status to file��6^  	 J� RenderTransformOrigin$	 0.5,0.5&�6�  $��	-10,0,0,0q�6�  �� 5      $��
Auto clear��6    $��20��65   $	��130��6A   $��0,0,0,0q�6M   $��Center=�6_   $��Arial)�6�   $
��Left=�6�   $��10(�6�   $��4,0,0,0q�6�   5      5      �� 5      -   5        
 ��Name$
 TextBox_SerialMonitor��5      $��Visible=�69   $
��Auto=�6l   $
��Left=�6�   $��2��6�   $��2��6�   $��4��6�    �� TextWrapping$
 Wrap=�6�   $�� ��6�   $	��Top=�6  4  .System.Windows.Automation.AutomationProperties   Name$ TextBox3��6  ���	#00252525���	#00CCCCCC$��0,0,0,0q�6o  $	��768��6�  $	��370��6�  $��5,0,0,0q�6�  5      5      5      5      
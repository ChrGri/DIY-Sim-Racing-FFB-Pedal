   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null7 clr-namespace:User.PluginSdkDemoUser.PluginSdkDemo  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles M7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.PluginsSimHub.Plugins.UI U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls &     User.PluginSdkDemo.OnlineProfile   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  
   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  
   @d2http://schemas.microsoft.com/expression/blend/2008  
   2local clr-namespace:User.PluginSdkDemo  
   Nstyles;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.Plugins  
   Fui7clr-namespace:SimHub.Plugins.UI;assembly=SimHub.Plugins  
   Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  
     � Title$  Online Profile Selection��$	��330��$	��405�� � WindowStyle$ 
ToolWindow=� � 
ResizeMode$ NoResize=���Z� .��� ���	#CC252525.+��� ���	#7F4E4E4E$	��305�� �� CornerRadius$ 5,5,5,5��$��0,2,0,0q�$	��390��$	��Top=�$
��Left=�._���  �� Orientation$ 
Horizontal=�$
��Left=�$	��390��.+��� $	��180��$	��305��.+��� -   $��ProfilesListBox��$	��160��$	��200��$	��Top=�$��10q�$
��Left=�$��10(�$��Arial)�+  %SimHub.Plugins.Styles.SHButtonPrimary  -     Name$ 	Btn_Apply��$	��160��$��Apply��$��25��$��10,0,0,0q�$
��Left=�$��	15,0,15,0q�$��Center=�  -   $ Btn_Online_profile_leave��$	��160��$��Leave without Change��$��25��$��	10,10,0,0q�$
��Left=�$��	15,0,15,0q�$��Center=��� $	��210��$	��305��.+��� $	��100��$��Center=�$��0,5,0,0q�.+��� -    ��Name$  canvas_Online_Config_curve��$	��100��$	��200�����	#7F20232F J� RenderTransformOrigin$ 0.5,0.5&�$
��Left=�$	��Top=�.+��� $��Center=�$	��Top=�$��FORCE��$��60�����	#7F7D7D7D$ 0.5,0.5&� �� Left$	 -25��
	 �� Top$	 40��$��Arial Black)�$��13��$��10(�
 �� TextAlignment$
 Center=�z�V� .~��� �� ��  �� Angle$	 -90Q�T� �� $��Center=�$��Center=�$��TRAVEL��$��60�����	#7F7D7D7D$ 0.5,0.5&�$ 70��$	 88��$��Arial Black)�$
 Center=�$��12��$��10(�� -    �Name$' !Polyline_Online_Config_ForceCurve��   AccentColorBrush#���   $	��200��$	��200�� J� Opacity$	 0.7Q� � Points= �    Zb -1 �� Zb ' �� h�	 �� �� ��$��1��$ 1,0.5&�$
	 -100��$
��Left=�$��Center=�z�V� .~���  �� ScaleX$ 1Q� �� ScaleY$ -1Q��� �� T�  T� X$ 0Q��� -    ��Name$ rect0_Online_Config��$��6��$��6��#���   $ -3��$	 97��$	 0.8Q�$��0���� -   $ rect1_Online_Config��$��6��$��6��#���   $ 37��$	 77��$	 0.8Q��� -   $ rect2_Online_Config��$��6��$��6��#���   $ 77��$	 57��$	 0.8Q��� -	   $ rect3_Online_Config��$��6��$��6��#���   $	 117��$	 37��$	 0.8Q��� -
   $ rect4_Online_Config��$��6��$��6��#���   $	 157��$	 17��$	 0.8Q��� -   $ rect5_Online_Config��$��6��$��6��#���   $	 197��$	 -3��$	 0.8Q��� -    ��Name$( "Textbox_Online_Profile_Description��$�� ��$	��200��$	��210��$��Arial)�$��Center=�$	��Top=�$��10(���. $��0,0,0,0q�" �� VerticalScrollBarVisibility$
 Auto=���Z� ��Z�  �� 
CaretBrush Z� 
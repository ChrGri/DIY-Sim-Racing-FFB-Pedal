   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullM+clr-namespace:User.PluginSdkDemo.UIFunctionUser.PluginSdkDemo.UIFunction  K*clr-namespace:User.PluginSdkDemo.UIElementUser.PluginSdkDemo.UIElement  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 8    2User.PluginSdkDemo.UIFunction.SettingSection_Pedal   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
 NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml  
   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006  
   @d2http://schemas.microsoft.com/expression/blend/2008  
   =local+clr-namespace:User.PluginSdkDemo.UIFunction  
   >localUI*clr-namespace:User.PluginSdkDemo.UIElement  
   Nstyles;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.Plugins  
   Pcontrols;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.Metro  
     � 	Resources     	TestBrush �� Color3  -System.Windows.Media.Effects.DropShadowEffect  mousedragging   Color   
BlurRadius   Opacity   ShadowDepth  mouseoutdragging  HorizontalSliderThumb �� 
TargetType �� Property  AccentColorBrush �� Value	 �� 
TargetType
 J� SnapsToDevicePixels �� RadiusX �� RadiusY ��Name  DisabledWhiteBrush J� IsHitTestVisible J� Opacity �� Triggers P� Property P� Value   HorizontalTrackLargeDecrease �Name �� EndPoint �� 
StartPoint �� Opacity � Color � Offset  HorizontalTrackValue  AccentColor 	 HorizontalSliderTemplate � RowDefinitions m�Name m� Fill m� 	Placement ]�Name ]� DecreaseRepeatButton �� DecreaseLarge  ]� IncreaseRepeatButton! �� IncreaseLarge" ]� Thumb'  !MahApps.Metro.Controls.MetroThumb#  Name 
 HorizontalThumbWidth$ �� 
TargetName  VerticalSliderThumb% �� BasedOn  VerticalTrackLargeDecrease  VerticalTrackValue  VerticalSliderTemplate& � ColumnDefinitions  VerticalThumbHeight  VerticalThumbWidth  SliderStyle_single_H  ControlBorderBrush  	TextBrush' �� Triggers  RangeSliderHorizontal(  "MahApps.Metro.Controls.RangeSlider( ��Name) �� Orientation* � Maximum+ m� Maximum, � Minimum- m� Minimum.   TickFrequency/ m� TickFrequency0 ��Name1 n�Name2 � Cursor3   MinRangeWidth  RangeSliderVertical  RangeSliderStyle�� %�  &	        &	      &	 ;     &	 e     &	     &	 m    &		 �    7  7  7  &	 �    7  &	 �    &	 ~    &	 �	    7  7  7  &	 �    &	 �    &	 �    &	 �    7  7  �� $ White��  $ White��$ 15Q�$ 1Q�$ 0Q�  $ White��$ 15Q�$ 0Q�$ 0Q��� " n�.���  � ��# �  ��  �@J�Opacity$	 0.9Q���  � ��$	 0 0q���  � �� .  ��  �@J�SnapsToDevicePixels . ��  � ��$ 10����  � �� �� "	 n�.R�� #
 z
 .+��� #��z��#��z��$ 0��$ 0���� $ DisabledVisualElement��#���   .  $ 0Q� P�  �@n�
IsDragging . . ��� # �   �@J�EffectP�  �@n�
IsDragging .  . ��� # �   �@J�Effect�� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� "	 ��.R�� $
 Root��$��0 -1 0 0q�.+��� $��1��$��0 1 0 0q�$ 1��$ 1������ $ 0.5,1&�$ 0.5,0&�$	 0.9Q�.��� # z��$	 0.0Q�� $
 Gray��$
 0.25Q�� $
 Gray��$
 0.75Q�� # z��$	 1.0Q��� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� "	 ��.R�� $
 Root��$��0 -1 0 0q�.+��� $��4��$��0 1 0 0q�$ 1��$ 1������ $ 0.5,1&�$ 0.5,0&�$	 0.9Q�.��� # z��$	 0.0Q�� # �  $
 0.25Q�� # �  $
 0.75Q�� # z��$	 1.0Q��� "	 ��.R�� $
 Root��.+�� $ HorizontalTemplate��#��z�� �� $
��Auto ��� $
��Auto �#��z���� $
��Auto �.+�m� $ TopTick��# z��$��4��$	 Top=�$��0��$y�	Collapsed=�m� $ 
BottomTick��# z��$��4��$ Bottom=�$��2��$y�	Collapsed=�]� $ 
PART_Track��$��1�� �� #��Z 8��    �� #��Z! 8�� "   $# HorizontalThumb��$��10����. 8�� #��� 
  P�  �@��TickPlacement$ TopLeft=�. ��� $$ TopTick�� � y�$ Visible=�P�  �@��TickPlacement$ BottomRight=�. ��� $$ 
BottomTick�� � y�$ Visible=�P�  �@��TickPlacement$
 Both=�. ��� $$ 
BottomTick�� � y�$ Visible=��� $$ TopTick�� � y�$ Visible=��� 8%   " n�.���  � ��$	 0 0q��� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� "	 ��.R�� $
 Root��$��-1 0 0 0q�.+��� 
�������$��1 0 0 0q�$��1��$	 0.8Q��� " ��.���  � |� .  ��  � �� .  ��  �@�OverridesDefaultStyle . ��  � �� �� "	 ��.R�� $
 Root��$��-1 0 0 0q�.+��� $��0 0 0 0q�$��10������ $ 1,0.5&�$ 0,0.5&�$	 0.9Q�.��� # z��$	 0.0Q�� # �  $
 0.25Q�� # �  $
 0.75Q�� # z��$	 1.0Q��� "	 ��.R�� $
 Root��.+�� $ HorizontalTemplate��#��z��& �� $
��Auto ��� #��z��$
��Auto ��� $
��Auto �.+�m� $ TopTick��$��0��# z��$
 Left=�$y�	Collapsed=�$��4��m� $ 
BottomTick��$��2��# z��$ Right=�$y�	Collapsed=�$��4��]� $ 
PART_Track��$��1�� �� #��Z 8��    �� #��Z! 8�� "   $# VerticalThumb��#���  ��. 8�� #���   P�  �@��TickPlacement$ TopLeft=�. ��� $$ TopTick�� � y�$ Visible=�P�  �@��TickPlacement$ BottomRight=�. ��� $$ 
BottomTick�� � y�$ Visible=�P�  �@��TickPlacement$
 Both=�. ��� $$ 
BottomTick�� � y�$ Visible=��� $$ TopTick�� � y�$ Visible=��� " ��.���  � ��# �  ��  � ��$ 1q���  � ��# �  ��  � �� .  ��  �@�Maximum$	 100Q���  �@�Minimum$ 0Q���  � ��# � 	 ��  �@�Value$ 0Q�' P�  �@��Orientation$ Vertical=�. ���  � ��# �  �� "	  .R��� $( PART_Container��$��Stretch=�$) Vertical=�$��Center=�.+�m� $ PART_TopTick��# z��$��4��$��5 0 5 2q�#+ z* #- z, $	 Top=�#/ z. $y�	Collapsed=��� $( PART_RangeSliderContainer��
������ $��Stretch=�$) 
Horizontal=�$��Center=�.+��� $0 PART_LeftEdge��#���  n� $1 PART_LeftThumb��$2 Arrow��$��10��$��10��#���  n� $1 PART_MiddleThumb��$
2 Hand��#��z3 ���� " n�.���  � �� �� .R��� #���  $��4��$��0 1 0 0q�$	 0.8Q�n� $1 PART_RightThumb��$2 Arrow��$��10��#���  $��10���� $0 PART_RightEdge��#���  m� $ PART_BottomTick��# z��$��4��$��5 2 5 0q�#+ z* #- z, $	 Top=�#/ z. $y�	Collapsed=� P�  �@ TickPlacement$ TopLeft=�. ��� $$ PART_TopTick�� � y�$ Visible=�P�  �@ TickPlacement$ BottomRight=�. ��� $$ PART_BottomTick�� � y�$ Visible=�P�  �@ TickPlacement$
 Both=�. ��� $$ PART_BottomTick�� � y�$ Visible=��� $$ PART_TopTick�� � y�$ Visible=��� "	  .R��� $( PART_Container��$��Center=�$) 
Horizontal=�$��Stretch=�.+�m� $ PART_TopTick��# z��$��0 5 2 5q�#+ z* #- z, $
 Left=�#/ z. $y�	Collapsed=�$��4���� $( PART_RangeSliderContainer��
������ $��Center=�$) Vertical=�$��Stretch=�.+��� $0 PART_RightEdge��#���  $��Stretch=�n� $1 PART_RightThumb��$2 Arrow��$��10��#���  $��10��n� $1 PART_MiddleThumb��$
2 Hand��#��z3 ���� " n�.���  � �� �� .R��� #���  $��0 0 0 0q�$��3��$	 0.7Q�n� $1 PART_LeftThumb��$2 Arrow��$��10��#���  $��10���� $0 PART_LeftEdge��#���  $��Stretch=�m� $ PART_BottomTick��# z��$��2 5 0 5q�#+ z* #- z, $ Right=�#/ z. $y�	Collapsed=�$��4�� P�  �@ TickPlacement$ TopLeft=�. ��� $$ PART_TopTick�� � y�$ Visible=�P�  �@ TickPlacement$ BottomRight=�. ��� $$ PART_BottomTick�� � y�$ Visible=�P�  �@ TickPlacement$
 Both=�. ��� $$ PART_BottomTick�� � y�$ Visible=��� $$ PART_TopTick�� � y�$ Visible=��� "  .���  � ��
 �������  � ��8   ' P�  �@ Orientation$ Vertical=�. ���  � ��8  .��� .+��� ���	#FF4E4E4E$	��280��4 �� CornerRadius$4 5,5,5,5��$��0,0,0,0q�$	��160��$
��Left=�$��0,3,0,0q�._��� $��Center=�$	��280��$	��Top=�$	��160��.+��� $��Pedal Setting��$��20��$	��150��$��0,0,0,0q�$��Center=�$��Arial Black)�$
��Left=�$��10(�$��2,0,0,0q��� $	��155��$��10��$) 
Horizontal=�$��2,1,0,0q�.+��� -   5 ��Name$"5 Label_Pedal_interval_trigger��$	��155��
�������6 J� RenderTransformOrigin$6 0.5,0.5&�
7 �� Top$7 3��$��8(�$��Arial)�8 �� Left$8 5��$
��Left=�$	��Top=�$��Action Interval:��$��10��$��0,0,0,0q��� -   9 ��Name$#9 Slider_Pedal_interval_trigger��#���  $
��Left=�$��1��$	��Top=�$	��155��$��10��$	* 100Q�: � SmallChange$: 1Q�; � LargeChange$; 10Q�< �� IsSnapToTickEnabled< . $, 20Q�= � ToolTip$V= PAdjust for the serial write interval, when meet I/O error, increase the interval��$��3,2,0,0q��� -   $( text_debug_flag��$��20��$	��150��$) 
Horizontal=�$
��Left=�$��Center=�$��0,2,0,0q�.+��� $��Debug flag 0:��$��20��$��60��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��5,0,0,0q��� -   > ��Name$> textBox_debug_Flag_0��$��40��$��20��$��2,0,0,0q�$
��Left=�$��20��$��8(�$>= 8DebugFlag input, please check ESP32 sketch for cheatcode��$��0,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�*  $SimHub.Plugins.Styles.SHToggleButton  -   ?  Name$? checkbox_auto_connect��$�� ��@ d� 	IsChecked$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$v= pAuto connect with scan the port every 5s, and connect back the pedal you have connect until you press disconnect��$6 0.5,0.5&�$��	-10,0,0,0q��� $��Auto Connect��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -   $? checkbox_pedal_read��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$0= *Auto Read Config from Pedal when connected��$6 0.5,0.5&�$��	-10,0,0,0q��� $��Auto Read Config��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -   $? CheckBox_LivePreview��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$�= �The preview is not save to pedal storage, please save config after you feel comfortable. The preview interval is binding with action interval��$6 0.5,0.5&�$��	-10,0,0,0q�{�.  �� $��Config Live Preview��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -   $$? CheckBox_Pedal_ESPNow_SyncFlag��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$6 0.5,0.5&�$��	-10,0,0,0q��� $��Wireless Comunication��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -	   $? CheckBox_USINGESP32S3��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$I= CEnable it for V4, and GIlphilbert 1.2 board, Disable it in V3 board��$6 0.5,0.5&�$��	-10,0,0,0q��� $��CDC Serial for ESP32S3��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -
   $!? dump_pedal_response_to_file��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$J= DDump pedal realtime status to file, it should be off when not in use��$6 0.5,0.5&�$��	-10,0,0,0q��� $��Log pedal states��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��20��$	��160��$) 
Horizontal=�$
��Left=�$��Center=�$��5,2,0,0q�.+�  -   $? Vjoy_out_check��$�� ��$@ FalseY�$��10(�$��35��$��15��$
��Left=�$��Arial)�$(= "Output the joystick value via vJoy��$6 0.5,0.5&�$��	-10,0,0,0q��� $��vJoy Output��$��20��$	��130��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��10(�$��4,0,0,0q��� $��15��$��80��$) 
Horizontal=�$
��Left=�$��Center=�$��35,2,0,0q�.+��� -   A ��Name$A btn_plus��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"��� -   $5 Label_vjoy_order��$��1��$��15��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=��� -   $A 	btn_minus��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�
   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullM+clr-namespace:User.PluginSdkDemo.UIFunctionUser.PluginSdkDemo.UIFunction  K*clr-namespace:User.PluginSdkDemo.UIElementUser.PluginSdkDemo.UIElement  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 7    1User.PluginSdkDemo.UIFunction.KinematicsTab_Pedal   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
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
 �������  � ��8   ' P�  �@ Orientation$ Vertical=�. ���  � ��8  .��� .+��� $
��Left=�$	��272��$	��Top=�$	��540�����	#7F4E4E4E4 �� CornerRadius$4 5,5,5,5��$��0,3,0,0q�._��� $
��Left=�$	��272��$	��Top=�$	��540��.+��� $��20��$	��540��$	��Top=�$
��Left=�$) 
Horizontal=�.+��� $��PEDAL KINEMATIC SETTING��$	��200��
�������$��Arial Black)�$��Center=�$��70,0,0,0q�$��20��$	��Top=�$��0,0,0,0q�$��Center=��� $	��205��$	��540��$) 
Horizontal=�$��Center=�$
��Left=�$��0,5,0,0q�.+��� -   5 ��Name$5 canvas_kinematic��$	��200��$	��400�����	#7F20232F6 J� RenderTransformOrigin$6 0.5,0.5&�$
��Left=�$	��Top=�$��70,0,0,0q�.+��� -   7 ��Name$"7 TextBlock_Warning_kinematics��$��40��$	��200��$��Arial)�$��9(����	#FFF44343�� -   8 ��Name$8 Line_kinematic_b��	9 �� X1$9 5��	: �� Y1$: 28��	; �� X2$	; 405��	< �� Y2$< 28��#���  $��5��= �� StrokeEndLineCap$= Round=�> �� StrokeStartLineCap$> Round=�$	 0.7Q��� -   $8 Line_kinematic_c_hort��$9 5��$: 28��$	; 405��$< 28��#���  $��5��$= Round=�$> Round=�$	 0.7Q��� -   $8 Line_kinematic_c_vert��$9 5��$: 28��$	; 405��$< 28��#���  $��5��$= Round=�$> Round=�$	 0.7Q��� -   $8 Line_kinematic_a��$9 5��$: 28��$	; 405��$< 28��#���  $��5��? �� StrokeDashCap$? Round=�$> Round=�$	 0.7Q��� -   $8 Line_kinematic_d��$9 5��$: 28��$	; 405��$< 28��#���  $��5��$= Round=�$> Round=�$	 0.7Q��� -   $8 Line_Pedal_Travel��$9 5��$: 28��$	; 405��$< 28�����	#CCFA8787$��5��$> Round=�$= Round=��� -	   $ rect_joint_O��$��10��$��10��#���  @ �� Left$@ -5��
A �� Top$	A 195��$	 0.9Q��� -
   $ rect_joint_A��$��10��$��10��#���  $@ -5��$	A 195��$	 0.9Q��� -   $ rect_joint_B��$��10��$��10��#���  $@ -5��$	A 195��$	 0.9Q��� -   $ rect_joint_C��$��10��$��10��#���  $@ -5��$	A 195��$	 0.9Q��� -   $ rect_joint_D��$��10��$��10��#���  $@ -5��$	A 195��$	 0.9Q��� -   B ��Name$B Label_joint_C��$��C��$��10��
�������$��Arial)�$��Center=�$��10��$	��Top=�$��0,0,0,0q�$��Center=�$��10(�$��Center=�$��Italic$��� -   $B Label_joint_A��$��A��$��10��
�������$��Arial)�$��Center=�$��10��$	��Top=�$��0,0,0,0q�$��Center=�$��10(�$��Center=�$��Italic$��� -   $B Label_joint_B��$��B��$��10��
�������$��Arial)�$��Center=�$��10��$	��Top=�$��0,0,0,0q�$��Center=�$��10(�$��Center=�$��Italic$��� -   $B Label_joint_D��$��D��$��10��
�������$��Arial)�$��Center=�$��10��$	��Top=�$��0,0,0,0q�$��Center=�$��10(�$��Center=�$��Italic$��� -   $B Label_joint_O��$��O��$��10��
�������$��Arial)�$��Center=�$��10��$	��Top=�$��0,0,0,0q�$��Center=�$��10(�$��Center=�$��Italic$��� -   $( SP_kinematic_b_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -   C ��Name$!C btn_plus_kinematic_b_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -   D ��Name$D Label_kinematic_b_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC$��0,-3,0,0q�E �� 
CaretBrush
E ���� $��0,0,0,0q��� -   $"C btn_minus_kinematic_b_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� -   $ ( SP_kinematic_c_hort_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -   $&C  btn_plus_kinematic_c_hort_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -   $#D Label_kinematic_c_hort_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC$��0,-3,0,0q�
E ���� $��0,0,0,0q��� -   $'C !btn_minus_kinematic_c_hort_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� -   $ ( SP_kinematic_c_vert_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -   $&C  btn_plus_kinematic_c_vert_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -   $#D Label_kinematic_c_vert_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC$��0,-3,0,0q�
E ���� $��0,0,0,0q��� -   $'C !btn_minus_kinematic_c_vert_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� -   $( SP_kinematic_a_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -    $!C btn_plus_kinematic_a_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -!   $D Label_kinematic_a_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC$��0,-3,0,0q�
E ���� $��0,0,0,0q��� -"   $"C btn_minus_kinematic_a_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� -#   $( SP_kinematic_d_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -$   $!C btn_plus_kinematic_d_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -%   $D Label_kinematic_d_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC$��0,-3,0,0q�
E ���� 
�������$��0,0,0,0q��� -&   $"C btn_minus_kinematic_d_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� -'   $( SP_travel_canvas��$��20��$��60��$) 
Horizontal=�$
��Left=�$��Center=�.+��� -(   $C btn_plus_travel_canvas��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"�$y�Hidden=��� -)   $D Label_travel_canvas��$��20��$��30��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=����	#00252525���	#00CCCCCC
�������
E ���� $��0,-3,0,0q�$��0,0,0,0q��� -*   $C btn_minus_travel_canvas��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(�$y�Hidden=��� $��60��$��15��$
��Left=�$��Center=�$��	385,0,0,0q�.+��� -+   $C btn_plus_kinematic_scale��$��+��$��15��$��15��$��0,0,0,0q�$��10��$��
0,-0.5,0,0q�$	��Top=�$
��Bold"��� -,   $B Label_kinematic_scale��$��15��$��15��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=��� --   $C btn_minus_kinematic_scale��$��-��$��15��$��15��$��0,0,0,0q�$��10��$��0,-3,0,0q�$��Center=�$
��Bold"�$��Yu Gothic UI)�$��Center=�$��10(��� $��15��$	��540��$) 
Horizontal=�$��Center=�$
��Left=�.+��� -.   $!B Label_kinematic_pedal_angle��$�� ��$��15��$	��400��$��Yu Gothic UI)�$��10(�$��Center=�$��0,0,0,0q�$��Center=�$��70,0,0,0q�
   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullM+clr-namespace:User.PluginSdkDemo.UIFunctionUser.PluginSdkDemo.UIFunction  K*clr-namespace:User.PluginSdkDemo.UIElementUser.PluginSdkDemo.UIElement  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls 3    -User.PluginSdkDemo.UIFunction.CurveTab_Rudder   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
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
��Left=�$	��272��$	��Top=�$	��540�����	#7F4E4E4E4 �� CornerRadius$4 5,5,5,5��._��� $
��Left=�$	��272��$	��Top=�$	��540��.+��� $��20��$	��540��$	��Top=�$
��Left=�$) 
Horizontal=�.+��� $��FORCE CURVE ADJUSTMENT��$	��200��
�������$��Arial Black)�$��Center=�$��70,0,0,0q�$��20��$	��Top=�$��0,0,0,0q�$��Center=�+  %SimHub.Plugins.Styles.SHButtonPrimary  -   5  Name$5 btn_linearcurve_rudder��$��40��$��Linear��$��15��$��0,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Arial)�$��10(�$��15��$��Center=�  -   $5 btn_Scurve_rudder��$��45��$��S-Curve��$��15��$��5,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��15��  -   $5 btn_10xcurve_rudder��$��50��$��Exponent��$��15��$��5,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��15��  -   $5 btn_logcurve_rudder��$��50��$��	Logarithm��$��15��$��5,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Arial)�$��10(�$��15��$��Center=��� $	��220��$	��540��$) 
Horizontal=�$��Center=�$
��Left=�$��0,5,0,0q�.+��� -   6 ��Name$6 canvas_rudder_curve��$	��200��$	��400�����	#7F20232F7 J� RenderTransformOrigin$7 0.5,0.5&�$
��Left=�$	��Top=�$��70,0,0,0q�z�V� .~��� 8 �� ScaleX$8 1Q�9 �� ScaleY$9 1Q��� �� T� .+��� $��Center=�$	��Top=�$��FORCE��$��60�����	#7F7D7D7D$7 0.5,0.5&�: �� Left$	: -25��
; �� Top$; 85��$��Arial Black)�$��13��z�V� .~��� �� �� < �� Angle$	< -90Q�T� �� $��Center=�$��Center=�$��TRAVEL��$��60�����	#7F7D7D7D$7 0.5,0.5&�$	: 170��$	; 186��$��Arial Black)�= �� TextAlignment$= Center=�$��14��� -   > �Name$> Polyline_RudderForceCurve��#���  $	��300��$	��400��$	 0.7Q�? � Points=? �    �� -1 h�	 Zb N �� � �� �� ��$��4��$7 1,0.5&�$
; -100��$
��Left=�$��Center=�z�V� .~��� $8 1Q�$9 -1Q��� �� T� @ T� X$@ 0Q��� -   A ��Name$A text_point_pos_rudder��$
��Left=�$	��Top=�$	��pos��$��60��
�������$7 0.5,0.5&�$: 5��$; 15��$��30��$��10(�$��Arial)��� -   $ rect0_rudder��$��10��$��10��#���  $: -5��$	; 195��$	 0.8Q�$��0���� -	   $ rect1_rudder��$��10��$��10��#���  $: 75��$	; 155��$	 0.8Q��� -
   $ rect2_rudder��$��10��$��10��#���  $	: 155��$	; 115��$	 0.8Q��� -   $ rect3_rudder��$��10��$��10��#���  $	: 235��$; 75��$	 0.8Q��� -   $ rect4_rudder��$��10��$��10��#���  $	: 315��$; 35��$	 0.8Q��� -   $ rect5_rudder��$��10��$��10��#���  $	: 395��$; -5��$	 0.8Q��� -   B ��Name$B text_rudder_log��$
��Left=�$	��Top=�$�� ��$	��400��
�������$7 0.5,0.5&�$: 0��$; 0��$	��100��$��10(�$��Arial)�$��0,0,0,0q�C �� 
IsReadOnlyC . $y�Hidden=���Z� ��Z� D �� 
CaretBrushD Z� E �� SelectionTextBrushE Z� �� $��50��$	��230��$��
10,-10,0,0q�.+��� -   F ��Name$F Label_max_force_rudder��$��50��$��24��
�������$7 0.5,0.5&�$; 3��$��10(�$��Arial)�$: 5��$
��Left=�$	��Top=�$	��Min��$��0,0,0,0q�$��0,-9,0,0q�  -   G  Name$$G Rangeslider_rudder_force_range��#���  $3 1Q�$��2��$	��180��H   MinRange$H 3Q�I   IsSnapToTickEnabledI . J   
UpperValue$J 10Q�$��20��K   Orientation$K Vertical=�$* 30Q�$
��Left=�L � LargeChange$	L 0.1Q�$, 1Q�M   
LowerValue$M 1Q�N � Value$N 1Q��� -   $F Label_min_force_rudder��$��50��$��24��
�������$7 0.5,0.5&�$; 3��$��10(�$��Arial)�$: 5��$
��Left=�$	��Top=�$	��Min��$��0,0,0,0q��� $	��430��$��40��$) 
Horizontal=�$��	0,-10,0,0q�.+��� -   $F Label_min_pos_rudder��$��30��$��40��
�������$7 0.5,0.5&�$; 3��$��10(�$��Arial)�$: 5��$
��Left=�$	��Top=�$	��Min��$��0,0,0,0q�$��Center=�  -   $%G Rangeslider_rudder_travel_range��#���  $3 1Q�$��2��$��20��$H 1Q�O � SmallChange$O 1Q�I . $J 95Q�$	��370��$	��Top=�$M 5Q�$, 5Q�$* 95Q�$N 85Q��� -   $F Label_max_pos_rudder��$��30��$��40��
�������$7 0.5,0.5&�$; 3��$��10(�$��Arial)�$: 5��$
��Left=�$	��Top=�$	��Min��$��0,0,0,0q�$��Center=�
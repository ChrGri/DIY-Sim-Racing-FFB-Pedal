   M S B A M L   `   `   `  ���� I  EDiyActivePedal, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullM+clr-namespace:User.PluginSdkDemo.UIFunctionUser.PluginSdkDemo.UIFunction  K*clr-namespace:User.PluginSdkDemo.UIElementUser.PluginSdkDemo.UIElement  P LSimHub.Plugins, Version=1.0.9214.36797, Culture=neutral, PublicKeyToken=nullU;clr-namespace:SimHub.Plugins.Styles;assembly=SimHub.PluginsSimHub.Plugins.Styles U QMahApps.Metro, Version=1.5.0.23, Culture=neutral, PublicKeyToken=f4fb5a3c4d1e5b4fV;clr-namespace:MahApps.Metro.Controls;assembly=MahApps.MetroMahApps.Metro.Controls :    4User.PluginSdkDemo.UIFunction.SystemSetting_Profiles   R NAvalonDock, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4N JWoteverLocalization, Version=1.0.0.0, Culture=neutral, PublicKeyToken=nullR NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35T PWoteverCommon, Version=1.1.0.0, Culture=neutral, PublicKeyToken=35893a067a282a97\ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35`	 \AvalonDock.Themes.VS2013, Version=4.72.1.0, Culture=neutral, PublicKeyToken=3e4669d2f30244f4L 9http://schemas.microsoft.com/winfx/2006/xaml/presentation       	 R
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
��Left=�$	��218��$	��Top=�$	��542��$��1��$��5��$) 
Horizontal=�.+��� -   4 ��Name$4 
ProfileTab��$��1��$
��Left=�$	��180��$��5��$	��Top=�$	��102��$��0,0,0,0q�$��0,0,0,0q����	#00252525
������ 5 �� TabStripPlacement$
5 Left=�$��Center=�$��0,3,0,0q�  6 �� CornerRadius7 ��Name8 ]� 
Conditions9 �� 
SourceName: �� Property; �� Value��%"  '��       �� " ��.���  � �� �� "	 ��.R��� $��Border��$��0,0,0,0q�
�������$6 5,5,5,5��$��0,0,2,2q�._��� $7 ContentSite��$��Center=�$
��Left=�$��Header��$	��0,0q� P�  �@��
IsSelected . . ��� $$ Border�� � �� �	#7F4E4E4E��  � �� �	#FFFFFFFFP�  �@��
IsSelected .  . ��� $$ Border�� � �� �	#334E4E4E��  � �� �	#7FFFFFFF]� 8 �� $9 Border��: �@J�IsMouseOver; . �� : �@��
IsSelected; .  ./��� $$ Border�� � �� �	#4C4E4E4E.@��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot A��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot B��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot C��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot D��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot E��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $	��100��$��30��$��0,0,0,0q����� $	��100��$��30��$��0,0,0,0q�._��� $��Slot F��$��Arial Black)�$��12(�$��30��$	��100��$��0,0,0,0q�$��Center=�$
��Left=�$��Center=�$��Center=��� $
��Left=�$	��178��$	��Top=�$	��438��$��1��$��5��$��0,5,0,0q�.+��� ���	#7F4E4E4E$6 5,5,5,5��$	��178��$��0,0,0,0q�$	��438��$
��Left=�._��� $
��Left=�$	��190��$	��Top=�$	��440��$��1��$��5��.+��� $
��Left=�$��30��$	��Top=�$	��440��$��1��$��5��$) 
Horizontal=�.+��� $��Profile Name:��$��15��$��90��$��0,0,0,0q�$��Center=�$��Arial Black)�$
��Left=�$��2,0,0,0q��� -   < ��Name$< textbox_profile_name��$�� ��$��20��$	��150��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��16(�$��0,0,0,0q�$��Italic$��� $
��Left=�$��20��$��Center=�$	��440��$��1��$��5��$) 
Horizontal=�.+��� -   = ��Name$= Clutch_file_check��$��20��$��20��> d� 	IsChecked$> FalseY�$�� ��$��8(�$��2,0,0,0q�{�.  �� $��Clutch:��$��15��$��50��$��0,0,0,0q�$��Center=�$��Arial Black)�$
��Left=�$��Center=��� -   ? ��Name$? Label_clutch_file��$�� ��$��20��$	��295��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��8(�$��Center=�+  %SimHub.Plugins.Styles.SHButtonPrimary  -   @  Name$@ Reading_clutch��$��30��$
��Bind��$��20��$��0,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20��  -   $@ Clear_clutch��$��30��$��Clear��$��20��$��2,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20���� -   $!( StackPanel_Effects_Status_0��$
��Left=�$��20��$��Center=�$	��440��$��1��$��5��$) 
Horizontal=��� $
��Left=�$��20��$	��Top=�$	��439��$��1��$��5��$) 
Horizontal=�.+��� -   $= Brake_file_check��$��20��$��20��$> FalseY�$�� ��$��8(�$��2,0,0,0q�{�.  �� $��Brake:��$��15��$��50��$��0,0,0,0q�$��Center=�$��Arial Black)�$
��Left=��� -	   $? Label_brake_file��$�� ��$��20��$	��295��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��8(�  -
   $@ Reading_brake��$��30��$
��Bind��$��20��$��0,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20��  -   $@ Clear_brake��$��30��$��Clear��$��20��$��2,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20���� -   $!( StackPanel_Effects_Status_1��$
��Left=�$��20��$��Center=�$	��440��$��1��$��5��$) 
Horizontal=��� $
��Left=�$��20��$	��Top=�$	��440��$��1��$��5��$) 
Horizontal=�.+��� -   $= Gas_file_check��$��20��$��20��$> FalseY�$�� ��$��8(�$��2,0,0,0q�{�.  �� $
��Gas:��$��15��$��50��$��0,0,0,0q�$��Center=�$��Arial Black)�$
��Left=��� -   $? Label_gas_file��$�� ��$��20��$	��295��$��0,0,0,0q�$��Center=�$��Arial)�$
��Left=�$��8(�  -   $@ Reading_gas��$��30��$
��Bind��$��20��$��0,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20��  -   $@ 	Clear_gas��$��30��$��Clear��$��20��$��2,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20���� -   $!( StackPanel_Effects_Status_2��$
��Left=�$��20��$��Center=�$	��440��$��1��$��5��$) 
Horizontal=��� $
��Left=�$��20��$	��Top=�$	��440��$��1��$��5��$) 
Horizontal=�$��0,3,0,0q�.+�  -   $@ btn_apply_profile��$��62��$��Apply Profile��$��20��$��	303,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20��  -   $@ btn_send_profile��$��62��$��Send Profile��$��20��$��2,0,0,0q�$��Center=�$��Center=�$��0,0,0,0q�$��Center=�$��Arial)�$��10(�$��20��
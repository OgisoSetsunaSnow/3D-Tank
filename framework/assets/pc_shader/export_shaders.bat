@echo off
REM usage: export_shaders.bat > .\Shaders.h

echo #ifdef WIN32
python sourceToHeader.py opaque_basic.hlsl Shader_OpqBasic
python sourceToHeader.py instance_3d.hlsl Shader_Instance3D
python sourceToHeader.py instance_2d.hlsl Shader_Instance2D
python sourceToHeader.py skybox.hlsl Shader_Skybox
echo #endif
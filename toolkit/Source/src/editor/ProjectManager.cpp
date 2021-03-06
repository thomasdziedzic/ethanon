/*--------------------------------------------------------------------------------------
 Ethanon Engine (C) Copyright 2008-2012 Andre Santee
 http://www.asantee.net/ethanon/

	Permission is hereby granted, free of charge, to any person obtaining a copy of this
	software and associated documentation files (the "Software"), to deal in the
	Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the
	following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------------*/

#include "ProjectManager.h"
#include "EditorCommon.h"
#include <unicode/utf8converter.h>
#include "../engine/Scene/ETHScene.h"
#include "../engine/ETHEngine.h"

#define _S_NEW_PROJ L"New project..."
#define _S_LOAD_PROJ L"Load project..."

#include <direct.h>

ProjectManager::ProjectManager(ETHResourceProviderPtr provider) :
	EditorBase(provider)
{
}

void ProjectManager::LoadEditor()
{
	CreateFileMenu();
	// get current program path
	m_curve = m_provider->GetVideo()->CreateSprite(utf8::c(GetProgramPath() + "data/curve_right.png").wc_str());
}

void ProjectManager::SaveLastDir(const char *szLastDir)
{
	if (!szLastDir)
	{
		return;
	}
	SaveAttributeToInfoFile(GetProgramPath(), "directories", "lastProject", szLastDir);
}

string ProjectManager::ReadLastDir()
{
	return GetAttributeFromInfoFile(GetProgramPath(), "directories", "lastProject");
}

void ProjectManager::CreateFileMenu()
{
	m_fileMenu.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), _S_MENU_TITLE, m_menuSize*m_menuScale, m_menuWidth*2, true);
	m_fileMenu.AddButton(_S_NEW_PROJ);
	m_fileMenu.AddButton(_S_LOAD_PROJ);
}

string ProjectManager::DoEditor(SpritePtr pNextAppButton)
{
	GSGUI_BUTTON file_r = PlaceFileMenu();

	if (file_r.text == _S_NEW_PROJ)
	{
		SaveAs();
	}
	if (file_r.text == _S_LOAD_PROJ)
	{
		Open();
	}

	SetFileNameToTitle(m_provider->GetVideo(), _ETH_PROJECT_MANAGER_WINDOW_TITLE);

	if (GetCurrentProject() == "")
	{
		ShadowPrint(Vector2(64,128),
			L"-Start a new project or load an existing one.\n" \
			L"-If you start a new project, all necessary files and paths will be copied to the target folder.\n" \
			L"-Media files (sprites, sounds, normals) should be manually placed into their correct directories.\n" \
			L"-Create .ENT entity files before building your scenes.\n");
	}
	else
	{
		wstring currentProjectFilename = 
			utf8::c(ETHGlobal::GetFileName(GetCurrentProject())).wc_str();
			wstring sText = L"Project loaded: " + currentProjectFilename + L"\n" +
			L"We're ready to go. Press TAB or click the arrow to go to the Editors";
		ShadowPrint(Vector2(64,128), sText.c_str());
	}

	return GetCurrentProject();
}

bool ProjectManager::SaveAs()
{
	char filter[] = "Ethanon Project files (*.ethproj)\0*.ethproj\0\0";
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	string sLastDir = ReadLastDir();
	if (SaveForm(filter, "", path, file, sLastDir.c_str()))
	{
		string sOut;
		AddExtension(path, ".ethproj", sOut);
		std::ofstream ofs(sOut.c_str());
		if (ofs.is_open())
		{
			ofs << "Ethanon Engine project file";
			ofs.close();
		}
		SetCurrentFile(sOut.c_str());
		SetCurrentProject(sOut.c_str());
		SaveLastDir(ETHGlobal::GetPathName(sOut.c_str(), true).c_str());
		PrepareProjectDir();
	}
	return true;
}

bool ProjectManager::Save()
{
	// dummy for a while because project files don't hold any internal data yet
	return true;
}

bool ProjectManager::Open()
{
	char filter[] = "Ethanon Project files (*.ethproj)\0*.ethproj\0\0";
	char path[512], file[512];
	string sLastDir = ReadLastDir();
	if (OpenForm(filter, "", path, file, sLastDir.c_str()))
	{
		SetCurrentProject(path);
		SetCurrentFile(path);
		SaveLastDir(ETHGlobal::GetPathName(path, true).c_str());
	}
	return true;
}

void ProjectManager::PrepareProjectDir()
{
	const ETH_FILE_TO_COPY fileToCopy[] = 
	{
		{"data/default_nm.png", true},
		{"data/defaultVS.cg", true},
		{"data/dynaShadowVS.cg", true},
		{"data/hAmbientVS.cg", true},
		{"data/hPixelLightDiff.cg", true},
		{"data/hPixelLightSpec.cg", true},
		{"data/hPixelLightVS.cg", true},
		{"data/hVertexLightShader.cg", true},
		{"data/vAmbientVS.cg", true},
		{"data/vPixelLightDiff.cg", true},
		{"data/vPixelLightSpec.cg", true},
		{"data/vPixelLightVS.cg", true},
		{"data/vVertexLightShader.cg", true},
		{"data/particleVS.cg", true},
		{"data/shadow.dds", true},
		{"data/shadow.png", true},
		{"data/Verdana14_shadow.fnt", true},
		{"data/Verdana14_shadow_0.png", true},
		{"data/Verdana20.fnt", true},
		{"data/Verdana20_0.png", true},
		{"data/Verdana20_shadow.fnt", true},
		{"data/Verdana20_shadow.png", true},
		{"data/Verdana24.fnt", true},
		{"data/Verdana24_0.png", true},
		{"data/Verdana24_shadow.fnt", true},
		{"data/Verdana24_shadow.png", true},
		{"data/Verdana30.fnt", true},
		{"data/Verdana30_0.png", true},
		{"data/Verdana30_shadow.fnt", true},
		{"data/Verdana30_shadow.png", true},
		{"data/Verdana64.fnt", true},
		{"data/Verdana64_0.png", true},
		{"data/Verdana64_1.png", true},
		{"data/Verdana64_shadow.fnt", true},
		{"data/Verdana64_shadow_0.png", true},
		{"data/Verdana64_shadow_1.png", true},
		{"effects/readme.txt", true},
		{"entities/readme.txt", true},
		{"entities/normalmaps/readme.txt", true},
		{"particles/readme.txt", true},
		{"particles/particle.png", true},
		{"scenes/readme.txt", true},
		{"soundfx/readme.txt", true},
		{"app.enml", true},
		{"machine.exe", true},
		{"audiere.dll", true},
		{"cg.dll", true},
		{"cgD3D9.dll", true},
		{"d3dx9_42.dll", true},
		{utf8::converter(_ETH_DEFAULT_MAIN_SCRIPT_FILE).str(), false},
		{"eth_util.angelscript", false},
		{"Collide.angelscript", true},
	};

	const string programPath = utf8::c(m_provider->GetProgramPath()).c_str();
	const string sProjectPath = GetCurrentProjectPath(true);
	const unsigned int size = sizeof(fileToCopy)/sizeof(ETH_FILE_TO_COPY);
	for (unsigned int t=0; t<size; t++)
	{
		const string sNewFolder = sProjectPath + fileToCopy[t].file;
		_mkdir(ETHGlobal::GetPathName(sNewFolder.c_str(), true).c_str());
		ETHGlobal::_MoveFile( utf8::c(programPath + fileToCopy[t].file).wstr(), utf8::c(sNewFolder).wstr(), fileToCopy[t].overwrite);
	}
}

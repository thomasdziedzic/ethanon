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

#include "../engine/Platform/FileListing.h"
#include "SceneEditor.h"
#include "EditorCommon.h"
#include <unicode/utf8converter.h>
#include "../engine/Entity/ETHRenderEntity.h"
#include <sstream>
#include <string>

#define _S_TOOL_MENU L"Tools"
#define _S_PLACE L"Place entity"
#define _S_SELECT L"Select entity"
#define _ENTITY_SELECTION_BAR_HEIGHT (48.0f)
#define _ENTITY_SELECTION_BAR_PROXIMITY (192.0f)

SceneEditor::SceneEditor(ETHResourceProviderPtr provider) :
	EditorBase(provider),
	m_camSpeed(400.0f), m_slidingAxis(0,0),
	m_slidingCamera(false),
	m_currentEntity(0)
{
	m_currentEntityIdx = 0;
	m_updateLights = false;
	m_genLightmapForThisOneOnly =-1;
	m_fullscreen = false;
	m_projManagerRequested = false;
	m_guiX = m_guiY = 0.0f;
	m_pSelected = 0;
}

SceneEditor::~SceneEditor()
{
	if (m_currentEntity)
	{
		m_currentEntity->Release();
	}
}

void SceneEditor::StopAllSoundFXs()
{
	m_pScene->ForceAllSFXStop();
}

bool SceneEditor::ProjectManagerRequested()
{
	const bool r = m_projManagerRequested;
	m_projManagerRequested = false;
	return r;
}

void SceneEditor::RebuildScene(const std::wstring& fileName)
{
	if (fileName != _ETH_EMPTY_SCENE_STRING)
		m_pScene = ETHScenePtr(new ETHScene(fileName, m_provider, true, ETHSceneProperties(), 0, 0, true, _ETH_SCENE_EDITOR_BUCKET_SIZE));
	else
		m_pScene = ETHScenePtr(new ETHScene(m_provider, true, ETHSceneProperties(), 0, 0, true, _ETH_SCENE_EDITOR_BUCKET_SIZE));
	m_sceneProps = *m_pScene->GetSceneProperties();
	m_pSelected = 0;
	m_genLightmapForThisOneOnly =-1;
	ResetForms();
	m_pScene->EnableLightmaps(true);

}

void SceneEditor::LoadEditor()
{
	CreateFileMenu();
	m_customDataEditor.LoadEditor(this);

	RebuildScene(_ETH_EMPTY_SCENE_STRING);
	m_provider->GetVideo()->SetBGColor(m_background);

	m_lSprite = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/l.png", 0xFFFF00FF);
	m_lSprite->SetOrigin(GSEO_CENTER);

	m_pSprite = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/p.png", 0xFFFF00FF);
	m_pSprite->SetOrigin(GSEO_CENTER);

	m_parallaxCursor = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/parallax.png", 0xFFFF00FF);
	m_parallaxCursor->SetOrigin(GSEO_CENTER);

	m_axis = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/axis.png");
	m_axis->SetOrigin(GSEO_CENTER);
	
	m_outline = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/outline.png");

	m_soundWave = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/soundwave.dds");
	m_soundWave->SetOrigin(GSEO_CENTER);

	m_invisible = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/invisible.png", 0xFFFF00FF);
	m_invisible->SetOrigin(GSEO_CENTER);

	m_arrows = m_provider->GetVideo()->CreateSprite(m_provider->GetProgramPath() + L"data/arrows.png");
	m_arrows->SetOrigin(GSEO_CENTER);

	m_renderMode.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, true, false, false);

	const std::string programPath = utf8::c(m_provider->GetProgramPath()).c_str();
	std::wstring rmStatus = utf8::c(GetAttributeFromInfoFile(programPath, "status", "renderMode")).wc_str();
	if (rmStatus != _S_USE_PS && rmStatus != _S_USE_VS)
	{
		rmStatus = _S_USE_PS;
	}
	m_renderMode.AddButton(_S_USE_PS, rmStatus == _S_USE_PS);
	m_renderMode.AddButton(_S_USE_VS, rmStatus == _S_USE_VS);

	m_panel.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, false, true, false);
	m_panel.AddButton(_S_GENERATE_LIGHTMAPS, false);
	m_panel.AddButton(_S_UPDATE_ENTITIES, false);
	m_panel.AddButton(_S_TOGGLE_STATIC_DYNAMIC, false);
	m_panel.AddButton(_S_LOCK_STATIC, true);

	// gets the last status saved into the info file
	string lmStatus = GetAttributeFromInfoFile(programPath, "status", "lightmapsEnabled");
	m_panel.AddButton(_S_USE_STATIC_LIGHTMAPS, ETHGlobal::IsTrue(lmStatus));

	// gets the last status saved into the info file
	lmStatus = GetAttributeFromInfoFile(programPath, "status", "showCustomData");
	m_panel.AddButton(_S_SHOW_CUSTOM_DATA, ETHGlobal::IsTrue(lmStatus));

	m_panel.AddButton(_S_SHOW_ADVANCED_OPTIONS, false);

	m_tool.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, true, false, false);
	m_tool.AddButton(_S_PLACE, false);
	m_tool.AddButton(_S_SELECT, true);

	for (unsigned int t=0; t<3; t++)
	{
		m_ambientLight[t].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 5, false);
		m_ambientLight[t].SetClamp(true, 0, 1);
		m_ambientLight[t].SetScrollAdd(0.1f);
		m_ambientLight[t].SetDescription(L"Scene ambient light");
	}
	m_ambientLight[0].SetText(L"Ambient R:");
	m_ambientLight[1].SetText(L"Ambient G:");
	m_ambientLight[2].SetText(L"Ambient B:");

	for (unsigned int t=0; t<2; t++)
	{
		m_zAxisDirection[t].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 5, false);
		m_zAxisDirection[t].SetClamp(false, 0, 0);
		m_zAxisDirection[t].SetScrollAdd(1.0f);
		m_zAxisDirection[t].SetDescription(L"Scene z-axis direction in screen space");
	}
	m_zAxisDirection[0].SetText(L"Z-axis dir x:");
	m_zAxisDirection[1].SetText(L"Z-axis dir y:");

	m_entityName.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth*2, 24, false);

	CreateEditablePosition();

	m_lightIntensity.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 5, false);
	m_lightIntensity.SetClamp(true, 0, 100.0f);
	m_lightIntensity.SetText(L"Light intensity:");
	m_lightIntensity.SetScrollAdd(0.25f);
	m_lightIntensity.SetDescription(L"Global light intensity");

	m_parallaxIntensity.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 5, false);
	m_parallaxIntensity.SetClamp(false, 0, 100.0f);
	m_parallaxIntensity.SetText(L"Parallax:");
	m_parallaxIntensity.SetScrollAdd(0.25f);
	m_parallaxIntensity.SetDescription(L"Parallax depth effect intensity");

	ResetForms();

	m_pScene->EnableLightmaps(true);

	ReloadFiles();
}

void SceneEditor::ResetForms()
{
	m_ambientLight[0].SetConstant(m_sceneProps.ambient.x);
	m_ambientLight[1].SetConstant(m_sceneProps.ambient.y);
	m_ambientLight[2].SetConstant(m_sceneProps.ambient.z);
	m_lightIntensity.SetConstant(m_sceneProps.lightIntensity);
	m_parallaxIntensity.SetConstant(m_sceneProps.parallaxIntensity);
	m_zAxisDirection[0].SetConstant(m_sceneProps.zAxisDirection.x);
	m_zAxisDirection[1].SetConstant(m_sceneProps.zAxisDirection.y);
}

string SceneEditor::DoEditor(SpritePtr pNextAppButton)
{
	m_provider->GetVideo()->SetCameraPos(m_v2CamPos);

	// update lightmap status
	const bool lmEnabled = m_pScene->AreLightmapsEnabled();
	m_pScene->EnableLightmaps(m_panel.GetButtonStatus(_S_USE_STATIC_LIGHTMAPS));

	const std::string programPath = utf8::c(m_provider->GetProgramPath()).c_str();

	// if the lightmaps status has changed, save it's status again
	if (lmEnabled != m_pScene->AreLightmapsEnabled())
	{
		SaveAttributeToInfoFile(programPath, "status", "lightmapsEnabled",
			(m_pScene->AreLightmapsEnabled()) ? "true" : "false");
	}

	// go to the next frame if the user presses the F key while selecting an entity
	if (m_pSelected)
	{
		if (m_pSelected->GetID() >= 0)
		{
			if (m_provider->GetInput()->GetKeyState(GSK_F) == GSKS_HIT)
			{
				if (m_pSelected)
				{
					unsigned int currentFrame = m_pSelected->GetFrame();
					currentFrame++;
					if (currentFrame >= m_pSelected->GetNumFrames())
					{
						currentFrame = 0;
					}
					m_pSelected->SetFrame(currentFrame);
				}
			}
		}
	}

	if (m_provider->GetInput()->GetKeyState(GSK_ESC) == GSKS_HIT)
	{
		m_pSelected = 0;
	}

	const int wheel = (int)m_provider->GetInput()->GetWheelState();
	const Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
	const Vector2 v2Screen = m_provider->GetVideo()->GetScreenSizeF();
	const bool guiButtonsFree = AreGUIButtonsFree(pNextAppButton);
	if (wheel != 0 && guiButtonsFree)
		SetPlacingMode();

	{
		const float fpsRate = m_provider->GetVideo()->GetFPSRate();
		const float camMoveSpeed = floor(m_camSpeed/fpsRate);
		if (!IsThereAnyFieldActive())
		{
			if (fpsRate > 0.0f)
			{
				if (m_provider->GetInput()->IsKeyDown(GSK_LEFT))
					m_provider->GetVideo()->MoveCamera(Vector2(-camMoveSpeed,0));
				if (m_provider->GetInput()->IsKeyDown(GSK_RIGHT))
					m_provider->GetVideo()->MoveCamera(Vector2(camMoveSpeed,0));
				if (m_provider->GetInput()->IsKeyDown(GSK_UP))
					m_provider->GetVideo()->MoveCamera(Vector2(0,-camMoveSpeed));
				if (m_provider->GetInput()->IsKeyDown(GSK_DOWN))
					m_provider->GetVideo()->MoveCamera(Vector2(0,camMoveSpeed));
			}
		}
		// border camera slide
		if (m_slidingCamera)
		{
			if (m_provider->GetInput()->GetMiddleClickState() == GSKS_HIT
				|| m_provider->GetInput()->GetLeftClickState() == GSKS_HIT
				|| m_provider->GetInput()->GetRightClickState() == GSKS_HIT)
			{
				m_slidingCamera = false;
			}
			if (fpsRate > 0.0f)
			{
				const Vector2 v2Dir = (m_slidingAxis-v2Cursor)*-(camMoveSpeed/200);
				m_provider->GetVideo()->MoveCamera(Vector2(floor(v2Dir.x), floor(v2Dir.y)));
			}
		}
		else if (m_provider->GetInput()->GetMiddleClickState() == GSKS_HIT && guiButtonsFree)
		{
			m_slidingCamera = true;
			if (m_slidingCamera)
			{
				m_slidingAxis = v2Cursor;
			}
		}
	}

	m_axis->Draw(Vector2(0,0));
	RenderScene();

	LoopThroughEntityList();

	if (m_tool.GetButtonStatus(_S_PLACE))
		PlaceEntitySelection();

	EntitySelector(guiButtonsFree);

	GSGUI_BUTTON file_r;
	if (!m_fullscreen)
		file_r = DoMainMenu();

	if (file_r.text == _S_NEW)
	{
		RebuildScene(_ETH_EMPTY_SCENE_STRING);		
		m_provider->GetVideo()->SetCameraPos(Vector2(0,0));
		SetCurrentFile(_BASEEDITOR_DEFAULT_UNTITLED_FILE);
	}
	if (file_r.text == _S_SAVE_AS)
	{
		SaveAs();
	}
	if (file_r.text == _S_SAVE || (m_provider->GetInput()->GetKeyState(GSK_CTRL) == GSKS_DOWN && m_provider->GetInput()->GetKeyState(GSK_S) == GSKS_HIT))
	{
		if (GetCurrentFile(true) == _BASEEDITOR_DEFAULT_UNTITLED_FILE)
			SaveAs();
		else
			Save();
	}
	if (file_r.text == _S_OPEN)
	{
		Open();
	}
	if (file_r.text == _S_GOTO_PROJ)
	{
		m_projManagerRequested = true;
	}
	if (CheckForFileUpdate())
	{
		OpenByFilename(GetCurrentFile(true).c_str());
	}

	if (guiButtonsFree && m_tool.GetButtonStatus(_S_PLACE))
		EntityPlacer();

	if (!m_fullscreen)
		DoStateManager();

	EditParallax();

	if (m_entityFiles.empty())
	{
		ShadowPrint(Vector2(256,400), L"There are no entities\nCreate your .ENT files first", L"Verdana24_shadow.fnt", GS_WHITE);
	}

	if (m_provider->GetInput()->GetKeyState(GSK_DELETE) == GSKS_HIT && guiButtonsFree)
	{
		if (m_pSelected)
		{
			const bool update = m_pSelected->HasShadow() || m_pSelected->HasLightSource();

			m_pSelected->ForceSFXStop();
			m_pScene->GetBucketManager().DeleteEntity(m_pSelected->GetID(), ETHGlobal::GetBucket(m_pSelected->GetPositionXY(), m_pScene->GetBucketSize()));
			m_pSelected = 0;
			if (m_pScene->GetNumLights() && update)
			{
				ShowLightmapMessage();
				m_updateLights = true;
			}
		}
	}

	SetFileNameToTitle(m_provider->GetVideo(), _ETH_MAP_WINDOW_TITLE);
	m_v2CamPos = m_provider->GetVideo()->GetCameraPos();
	DrawEntitySelectionGrid(pNextAppButton);

	#ifdef _DEBUG
		if (m_pSelected)
		{
			stringstream ss;
			ss << m_pSelected->GetID();

			m_provider->GetVideo()->DrawBitmapText(
				Vector2(100,100), 
				utf8::c(ss.str()).wstr(), L"Verdana20_shadow.fnt", GS_WHITE
			);
		}
	#endif

	if (m_slidingCamera)
	{
		m_arrows->Draw(m_slidingAxis+m_v2CamPos);
	}

	if (m_provider->GetInput()->IsKeyDown(GSK_CTRL) && m_provider->GetInput()->GetKeyState(GSK_C) == GSKS_HIT)
		CopySelectedToClipboard();
	if (m_provider->GetInput()->IsKeyDown(GSK_CTRL) && m_provider->GetInput()->GetKeyState(GSK_V) == GSKS_HIT)
		PasteFromClipboard();

	CentralizeShortcut();
	return "scene";
}

bool SceneEditor::CheckForFileUpdate()
{
	if (m_fileChangeDetector)
	{
		m_fileChangeDetector->Update();
		return m_fileChangeDetector->CheckForChange();
	}
	else
	{
		return false;
	}
}

void SceneEditor::CreateFileUpdateDetector(const str_type::string& fullFilePath)
{
	m_fileChangeDetector = ETHFileChangeDetectorPtr(
		new ETHFileChangeDetector(m_provider->GetVideo(), fullFilePath, ETHFileChangeDetector::UTF16_WITH_BOM));
	if (!m_fileChangeDetector->IsValidFile())
		m_fileChangeDetector.reset();
}

void SceneEditor::EditParallax()
{
	if (m_parallaxIntensity.IsActive())
	{
		const Vector2 &cursorPos = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
		m_provider->GetShaderManager()->SetParallaxNormalizedOrigin(cursorPos/m_provider->GetVideo()->GetScreenSizeF());
		m_parallaxCursor->Draw(cursorPos+m_provider->GetVideo()->GetCameraPos(), GS_COLOR(100,255,255,255));
	}
	else
	{
		m_provider->GetShaderManager()->SetParallaxNormalizedOrigin(Vector2(0.5f,0.5f));
	}
}

void SceneEditor::Clear()
{
	m_pScene->ClearResources();
	RebuildScene(_ETH_EMPTY_SCENE_STRING);
	m_provider->GetVideo()->SetCameraPos(Vector2(0,0));
	SetCurrentFile(_BASEEDITOR_DEFAULT_UNTITLED_FILE);
	m_currentEntityIdx = 0;
}

void SceneEditor::SetSelectionMode()
{
	m_tool.ActivateButton(_S_SELECT);
	m_tool.DeactivateButton(_S_PLACE);
	m_panel.HideButton(_S_LOCK_STATIC, false);
	m_panel.HideButton(_S_TOGGLE_STATIC_DYNAMIC, false);
}
void SceneEditor::SetPlacingMode()
{
	m_tool.ActivateButton(_S_PLACE);
	m_tool.DeactivateButton(_S_SELECT);
	m_panel.HideButton(_S_LOCK_STATIC, true);
	m_panel.HideButton(_S_TOGGLE_STATIC_DYNAMIC, true);
}

void SceneEditor::EntitySelector(const bool guiButtonsFree)
{
	if (!m_tool.GetButtonStatus(_S_SELECT))
		return;

	const bool zBuffer = m_provider->GetVideo()->GetZBuffer();
	m_provider->GetVideo()->SetZBuffer(false);
	m_provider->GetVideo()->SetSpriteDepth(0.0f);
	const bool rightClick = (m_provider->GetInput()->GetRightClickState() == GSKS_HIT);

	// is the user moving the entity around?
	static bool moving = false;

	// Looks for the target entity ID and selects it if it exists
	if ((m_provider->GetInput()->GetLeftClickState() == GSKS_RELEASE || rightClick) && guiButtonsFree)
	{
		const Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());

		const int id = m_pScene->GetBucketManager().SeekEntity(v2Cursor, (reinterpret_cast<ETHEntity**>(&m_pSelected)), m_sceneProps, m_pSelected);

		if (m_pSelected)
		{
			if (m_pSelected->IsTemporary())
			{
				m_pSelected = 0;
			}
			if (id < 0)
			{
				m_pSelected = 0;
			}
		}

		if (m_pSelected)
		{
			m_entityName.SetActive(false);
			m_entityName.SetValue(m_pSelected->GetEntityName());

			SetEditablePositionPos(m_pSelected->GetPosition(), m_pSelected->GetAngle());
			#ifdef _DEBUG
			cout << "(Scene editor)Selected entity: " << m_pSelected->GetID() << endl;
			#endif
		}
	}

	// handle selection
	if (m_pSelected)
	{
		// If the user is holding the left-button and the cursor is over the selected tile...
		// move it...
		if (m_provider->GetInput()->GetLeftClickState() == GSKS_DOWN)
		{
			const Vector2 v2CursorPos = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
			ETH_VIEW_RECT box = m_pSelected->GetScreenRect(*m_pScene->GetSceneProperties());
			if ((v2CursorPos.x > box.v2Min.x && v2CursorPos.x < box.v2Max.x &&
				v2CursorPos.y > box.v2Min.y && v2CursorPos.y < box.v2Max.y) || moving)
			{
				const bool lockStatic = m_panel.GetButtonStatus(_S_LOCK_STATIC);
				if ((m_pSelected->IsStatic() && !lockStatic) || !m_pSelected->IsStatic())
				{
					moving = true;
					m_pSelected->AddToPositionXY(m_provider->GetInput()->GetMouseMoveF(), m_pScene->GetBucketManager());
					SetEditablePositionPos(m_pSelected->GetPosition(), m_pSelected->GetAngle());
				}
			}
		}

		// draws the current selected entity outline if we are on the selection mode
		if (!moving)
		{
			ETH_VIEW_RECT box = m_pSelected->GetScreenRect(*m_pScene->GetSceneProperties());
			const GS_COLOR dwColor(96,255,255,255);
			m_outline->SetOrigin(GSEO_DEFAULT);
			const Vector2 v2Pos = box.v2Min+m_provider->GetVideo()->GetCameraPos();
			m_outline->DrawShaped(v2Pos, box.v2Max-box.v2Min, dwColor, dwColor, dwColor, dwColor);
			DrawEntityString(m_pSelected, GS_WHITE);

			ShadowPrint(Vector2(m_guiX,m_guiY), L"Entity name:"); m_guiY += m_menuSize;
			m_pSelected->ChangeEntityName(utf8::c(m_entityName.PlaceInput(Vector2(m_guiX,m_guiY))).wstr()); m_guiY += m_menuSize;
			m_guiY += m_menuSize/2;

			// assign the position according to the position panel
			m_pSelected->SetPosition(GetEditablePositionFieldPos(), m_pScene->GetBucketManager());
			m_pSelected->SetAngle(GetEditablePositionFieldAngle());
		}
	}

	// show custom data editing options
	m_guiY += m_customDataEditor.DoEditor(Vector2(m_guiX, m_guiY), m_pSelected, this);

	// draw the outline to the cursor entity
	if (!moving)
	{
		const Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
		ETHEntity *pOver;
		const int id = m_pScene->GetBucketManager().SeekEntity(v2Cursor, &pOver, m_sceneProps);
		if (id >= 0 && (!m_pSelected || m_pSelected->GetID() != id))
		{
			ETHRenderEntity *pSelected = reinterpret_cast<ETHRenderEntity*>(m_pScene->GetBucketManager().SeekEntity(id));
			if (pSelected)
			{
				ETH_VIEW_RECT box = pSelected->GetScreenRect(*m_pScene->GetSceneProperties());
				const GS_COLOR dwColor(16, 255, 255, 255);
				m_outline->SetOrigin(GSEO_DEFAULT);
				m_outline->DrawShaped(box.v2Min + m_provider->GetVideo()->GetCameraPos(), box.v2Max - box.v2Min, dwColor, dwColor, dwColor, dwColor);
				DrawEntityString(pSelected, GS_COLOR(100, 255, 255, 255));
			}
		}
	}

	// if the user is moving and has released the mouse button
	if (moving && m_provider->GetInput()->GetLeftClickState() == GSKS_RELEASE)
	{
		moving = false;
	}

	m_provider->GetVideo()->SetZBuffer(zBuffer);
}

void SceneEditor::CreateEditablePosition()
{
	for (unsigned int t=0; t<3; t++)
	{
		m_entityPosition[t].SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 10, false);
		m_entityPosition[t].SetClamp(false, 0, 0);
		m_entityPosition[t].SetScrollAdd(2.0f);
		m_entityPosition[t].SetDescription(L"Entity position");
	}
	m_entityPosition[0].SetText(L"Position.x:");
	m_entityPosition[1].SetText(L"Position.y:");
	m_entityPosition[2].SetText(L"Position.z:");

	m_entityAngle.SetupMenu(m_provider->GetVideo(), m_provider->GetInput(), m_menuSize, m_menuWidth, 10, false);
	m_entityAngle.SetClamp(false, 0, 0);
	m_entityAngle.SetScrollAdd(5.0f);
	m_entityAngle.SetDescription(L"Entity angle");
	m_entityAngle.SetText(L"Angle:");
}

bool SceneEditor::AreEditPosFieldsActive() const
{
	for (unsigned int t=0; t<3; t++)
	{
		if (m_entityPosition[t].IsActive())
			return true;
	}
	if (m_entityAngle.IsActive())
		return true;
	return false;
}

bool SceneEditor::AreEditPosFieldsFocusedByCursor() const
{
	for (unsigned int t=0; t<3; t++)
	{
		if (m_entityPosition[t].IsMouseOver())
			return true;
	}
	if (m_entityAngle.IsMouseOver())
		return true;
	return false;
}

float SceneEditor::PlaceEditablePosition(const Vector2 &v2Pos)
{
	ShadowPrint(v2Pos, L"Entity position:");
	for (unsigned int t=0; t<3; t++)
	{
		m_entityPosition[t].PlaceInput(v2Pos+Vector2(0,static_cast<float>(t+1)*m_menuSize), Vector2(0,0), m_menuSize);
	}
	m_entityAngle.PlaceInput(v2Pos+Vector2(0,4.0f*m_menuSize), Vector2(0,0), m_menuSize);
	return m_menuSize*5;
}

Vector3 SceneEditor::GetEditablePositionFieldPos() const
{
	const Vector3 r(m_entityPosition[0].GetLastValue(),
					   m_entityPosition[1].GetLastValue(),
					   m_entityPosition[2].GetLastValue());
	return r;
}

float SceneEditor::GetEditablePositionFieldAngle() const
{
	return m_entityAngle.GetLastValue();
}

void SceneEditor::SetEditablePositionPos(const Vector3 &pos, const float angle)
{
	const float *pPos = (float*)&pos;
	for (unsigned int t=0; t<3; t++)
	{
		m_entityPosition[t].SetConstant(pPos[t]);
	}
	m_entityAngle.SetConstant(angle);
}

wstring SceneEditor::DrawEntityString(ETHEntity *pEntity, const GS_COLOR dwColor, const bool drawName)
{
	ETH_VIEW_RECT box = pEntity->GetScreenRect(*m_pScene->GetSceneProperties());
	box.v2Min -= Vector2(0,m_menuSize);
	wstringstream sID;
	sID << pEntity->GetID() << " ";
	if (drawName)
		sID << utf8::c(pEntity->GetEntityName()).wc_str();
	ShadowPrint(box.v2Min, sID.str().c_str(), dwColor);

	wstringstream sPos;
	sPos << L"pos: " << ETHGlobal::Vector3ToString(pEntity->GetPosition()) << endl << ((pEntity->IsStatic()) ? L"[static]" : L"[dynamic]");
	ShadowPrint(box.v2Min+Vector2(0,m_menuSize), sPos.str().c_str(), GS_COLOR(dwColor.a/2,dwColor.r,dwColor.g,dwColor.b));
	return sID.str()+L"\n"+sPos.str();
}

bool SceneEditor::IsThereAnyFieldActive() const
{
	for (unsigned int t=0; t<3; t++)
	{
		if (m_ambientLight[t].IsActive() || m_entityPosition[t].IsActive())
			return true;
	}
	if (m_entityAngle.IsActive())
		return true;
	for (unsigned int t=0; t<2; t++)
	{
		if (m_zAxisDirection[t].IsActive() || m_zAxisDirection[t].IsActive())
			return true;
	}

	if (m_entityName.IsActive())
		return true;

	if (m_fileMenu.IsActive())
		return true;

	if (m_lightIntensity.IsActive())
		return true;

	if (m_parallaxIntensity.IsActive())
		return true;

	if (m_customDataEditor.IsFieldActive())
		return true;

	return false;
}

bool SceneEditor::AreGUIButtonsFree(SpritePtr pNextAppButton) const
{
	for (unsigned int t=0; t<3; t++)
	{
		if ((m_ambientLight[t].IsMouseOver() || m_ambientLight[t].IsActive()))
			return false;
		if (m_entityPosition[t].IsMouseOver() || m_entityPosition[t].IsActive())
			return false;
	}
	if (m_entityAngle.IsMouseOver() || m_entityAngle.IsActive())
		return false;
	for (unsigned int t=0; t<2; t++)
	{
		if ((m_zAxisDirection[t].IsMouseOver() || m_zAxisDirection[t].IsActive()))
			return false;
	}

	const Vector2 v2Screen = m_provider->GetVideo()->GetScreenSizeF();
	const Vector2 v2Size = (pNextAppButton) ? pNextAppButton->GetBitmapSizeF() : Vector2(32,32);
	const Vector2 v2Pos = v2Screen-v2Size;
	const Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());

	// if the cursor is over the selection bar...
	if (v2Cursor.y > v2Screen.y-_ENTITY_SELECTION_BAR_HEIGHT)
		return false;

	// if the cursor is over the tab area
	if (v2Cursor.y < m_menuSize*2)
		return false;

	// if the cursor is over any of the cCustomDataEditor object
	if (!m_customDataEditor.IsCursorAvailable())
		return false;

	// If the cursor is over it, don't let it place
	if (AreEditPosFieldsFocusedByCursor() || AreEditPosFieldsActive() || m_panel.IsMouseOver()
		|| m_entityName.IsActive()/* || m_entityName.IsMouseOver()*/)
		return false;

	return (!IsFileMenuActive() && !IsMouseOverFileMenu() && !m_fileMenu.IsActive()
			&& !m_renderMode.IsMouseOver()
			&& !m_lightIntensity.IsActive() && !m_lightIntensity.IsMouseOver()
			&& !m_parallaxIntensity.IsActive() && !m_parallaxIntensity.IsMouseOver()
			&& !m_tool.IsMouseOver() && (v2Cursor.x < v2Pos.x || v2Cursor.y < v2Pos.y));
}

GSGUI_BUTTON SceneEditor::DoMainMenu()
{
	GSGUI_BUTTON file_r = PlaceFileMenu();

	return file_r;
}

void SceneEditor::DoStateManager()
{
	if (m_provider->GetInput()->GetRightClickState() == GSKS_HIT)
	{
		SetSelectionMode();
	}

	m_guiX = m_provider->GetVideo()->GetScreenSizeF().x-m_menuWidth*2;
	m_guiY = m_menuSize+m_menuSize*2;
	m_guiY+=m_menuSize/2;

	wstringstream ss;
	const unsigned int nEntities = m_pScene->GetNumEntities();
	ss << L"Entities in scene: " << m_pScene->GetNumRenderedEntities() << L"/" << nEntities;
	ShadowPrint(Vector2(m_guiX, m_guiY), ss.str().c_str(), GS_COLOR(255,255,255,255)); m_guiY += m_menuSize;
	m_guiY += m_menuSize/2;

	ShadowPrint(Vector2(m_guiX,m_guiY), L"Lighting mode:"); m_guiY+=m_menuSize;
	m_renderMode.PlaceMenu(Vector2(m_guiX,m_guiY)); m_guiY += m_menuSize*m_renderMode.GetNumButtons();

	const bool usePS = m_provider->GetShaderManager()->IsUsingPixelShader();
	m_provider->GetShaderManager()->UsePS(m_renderMode.GetButtonStatus(_S_USE_PS));
	m_guiY+=m_menuSize/2;

	const std::string programPath = utf8::c(m_provider->GetProgramPath()).c_str();

	// if this button's status has changed, save the change to the ENML file
	if (usePS != m_provider->GetShaderManager()->IsUsingPixelShader())
	{
		const wstring str = m_provider->GetShaderManager()->IsUsingPixelShader() ? _S_USE_PS : _S_USE_VS;
		SaveAttributeToInfoFile(
			programPath, "status", "renderMode", utf8::c(str).c_str()
		);
	}

	ShadowPrint(Vector2(m_guiX, m_guiY), _S_TOOL_MENU); m_guiY+=m_menuSize;
	m_tool.PlaceMenu(Vector2(m_guiX, m_guiY)); m_guiY += m_menuSize * m_renderMode.GetNumButtons();
	m_guiY+=m_menuSize/2;

	const Vector2 v2DescPos(0.0f, m_provider->GetVideo()->GetScreenSizeF().y-m_menuSize);
	m_sceneProps.ambient.x = m_ambientLight[0].PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	m_sceneProps.ambient.y = m_ambientLight[1].PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	m_sceneProps.ambient.z = m_ambientLight[2].PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	m_sceneProps.lightIntensity = m_lightIntensity.PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	m_sceneProps.parallaxIntensity = m_parallaxIntensity.PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	m_guiY+=m_menuSize/2;
	if (m_panel.GetButtonStatus(_S_SHOW_ADVANCED_OPTIONS))
	{
		m_sceneProps.zAxisDirection.x = m_zAxisDirection[0].PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
		m_sceneProps.zAxisDirection.y = m_zAxisDirection[1].PlaceInput(Vector2(m_guiX,m_guiY), v2DescPos, m_menuSize); m_guiY += m_menuSize;
	}
	m_pScene->SetSceneProperties(m_sceneProps);
	m_guiY+=m_menuSize/2;

	// save the last button status to compare to the new one and check if it has changed
	const bool showCustomData = m_panel.GetButtonStatus(_S_SHOW_CUSTOM_DATA);

	m_panel.PlaceMenu(Vector2(m_guiX, m_guiY)); m_guiY += m_menuSize*m_panel.GetNumButtons();
	m_guiY+=m_menuSize/2;

	// if the button status has changed, save it
	if (showCustomData != m_panel.GetButtonStatus(_S_SHOW_CUSTOM_DATA))
	{
		SaveAttributeToInfoFile(programPath, "status", "showCustomData",
			(!showCustomData) ? "true" : "false");
	}

	// if the user clicked to update lightmaps...
	if (m_panel.GetButtonStatus(_S_GENERATE_LIGHTMAPS))
	{
		m_updateLights = true;
		ShowLightmapMessage();
		m_panel.DeactivateButton(_S_GENERATE_LIGHTMAPS);
	}

	// if the user clicked to update entities...
	if (m_panel.GetButtonStatus(_S_UPDATE_ENTITIES) || m_provider->GetInput()->GetKeyState(GSK_F5) == GSKS_HIT)
	{
		UpdateEntities();
		m_updateLights = true;
		ShowLightmapMessage();
		m_panel.DeactivateButton(_S_UPDATE_ENTITIES);
	}

	// if the user clicked to toggle form
	if (m_panel.GetButtonStatus(_S_TOGGLE_STATIC_DYNAMIC))
	{
		if (m_pSelected)
		{
			if (m_pSelected->IsStatic())
			{
				m_pSelected->TurnDynamic();
			}
			else
			{
				m_pSelected->TurnStatic();
			}

			// if it has a shadow, generate lightmaps again
			if (m_pSelected->HasShadow() || m_pSelected->HasLightSource())
			{
				if (!m_pSelected->HasShadow() && !m_pSelected->HasLightSource())
				{
					m_genLightmapForThisOneOnly = m_pSelected->GetID();
				}
				m_updateLights = true;
				ShowLightmapMessage();
			}
		}
		m_panel.DeactivateButton(_S_TOGGLE_STATIC_DYNAMIC);
	}

	if (m_tool.GetButtonStatus(_S_PLACE) && m_currentEntity)
	{
		ShadowPrint(
			Vector2(m_guiX,m_guiY), 
			utf8::c(m_currentEntity->GetEntityName()).wc_str()
		); 
		m_guiY+=m_menuSize;
	}

	if (m_pSelected && m_tool.GetButtonStatus(_S_SELECT))
	{
		// if the current selected entity is valid, draw the fields
		if (m_pScene->GetBucketManager().SeekEntity(m_pSelected->GetID()))
		{
			m_guiY +=m_menuSize/2;
			m_guiY += PlaceEditablePosition(Vector2(m_guiX,m_guiY));
			m_guiY +=m_menuSize/2;
		}
	}

}

void SceneEditor::DrawGrid()
{
	int sizeX = (int)m_currentEntity->GetCurrentSize().x;
	int sizeY = (int)m_currentEntity->GetCurrentSize().y;
	const float oldWidth = m_provider->GetVideo()->GetLineWidth();
	const Vector2 v2Screen = m_provider->GetVideo()->GetScreenSizeF();
	const Vector2 v2CamPos = m_provider->GetVideo()->GetCameraPos();
	m_provider->GetVideo()->SetCameraPos(Vector2(0,0));
	const GS_COLOR dwLineColor = ARGB(70,255,255,255);
	m_provider->GetVideo()->SetLineWidth(2);

	Vector2 v2PosFix;
	if (m_currentEntity->GetType() == ETH_VERTICAL)
	{
		sizeY = (int)m_currentEntity->GetCurrentSize().x;
	}
	else
	{
		sizeY = (int)m_currentEntity->GetCurrentSize().y;
	}
	v2PosFix.x = float((int)v2CamPos.x%(sizeX));
	v2PosFix.y = float((int)v2CamPos.y%(sizeY));

	// draw vertical lines
	for (unsigned int t=0;; t++)
	{
		const Vector2 a((float)t*(float)sizeX-v2PosFix.x, 0);
		const Vector2 b((float)t*(float)sizeX-v2PosFix.x, v2Screen.y);
		m_provider->GetVideo()->DrawLine(a, b, dwLineColor, dwLineColor);
		if (a.x > v2Screen.x)
			break;
	}

	// draw horizontal lines
	for (unsigned int t=0;; t++)
	{
		const Vector2 a(0, (float)t*(float)sizeY-v2PosFix.y);
		const Vector2 b(v2Screen.x, (float)t*(float)sizeY-v2PosFix.y);
		m_provider->GetVideo()->DrawLine(a, b, dwLineColor, dwLineColor);
		if (a.y > v2Screen.y)
			break;
	}

	m_provider->GetVideo()->SetLineWidth(oldWidth);
	m_provider->GetVideo()->SetCameraPos(v2CamPos);
}

void SceneEditor::PlaceEntitySelection()
{
	const ETHEntityProperties *pData = m_currentEntity->GetProperties();
	if (m_currentEntityIdx < 0)
	{
		return;
	}

	if (m_entityFiles.empty())
	{
		return;
	}

	// select between entity list with home/end buttons
	if (m_provider->GetInput()->GetKeyState(GSK_HOME) == GSKS_HIT)
		m_currentEntityIdx--;
	if (m_provider->GetInput()->GetKeyState(GSK_END) == GSKS_HIT)
		m_currentEntityIdx++;

	const int wheelValue = static_cast<int>(m_provider->GetInput()->GetWheelState());

	// is the mouse wheel available for entity switching?
	const bool allowSwapWheel = AreGUIButtonsFree(SpritePtr());

	if (allowSwapWheel)
	{
		m_currentEntityIdx += wheelValue;

		if (m_currentEntityIdx >= static_cast<int>(m_entityFiles.size()))
			m_currentEntityIdx = 0;
		if (m_currentEntityIdx < 0)
			m_currentEntityIdx = m_entityFiles.size() - 1;

		// move the entity up and down (along the Z axis)
		if (m_provider->GetInput()->GetKeyState(GSK_PAGEUP) == GSKS_HIT)
		{
			m_v3Pos.z+=4;
		}
		if (m_provider->GetInput()->GetKeyState(GSK_PAGEDOWN) == GSKS_HIT)
		{
			m_v3Pos.z-=4;
		}

		// if the user has changed the current entity, return it's height to 0
		if (wheelValue != 0)
		{
			m_v3Pos.z = 0.0f;
		}
	}
	// Reset the current entity according to the selected one
	string currentProjectPath = GetCurrentProjectPath(true);

	// save the current angle to restore after entity reset
	const float angle = (m_currentEntity) ? m_currentEntity->GetAngle() : 0.0f;

	if (m_currentEntity)
		m_currentEntity->Release();

	m_currentEntity = new ETHRenderEntity(m_provider, *m_entityFiles[m_currentEntityIdx].get(), angle, 1.0f);
	m_currentEntity->SetOrphanPosition(m_v3Pos);
	m_currentEntity->SetFrame(m_entityFiles[m_currentEntityIdx]->startFrame);
	m_currentEntity->SetAngle(angle);

	Vector2 v2CamPos = m_provider->GetVideo()->GetCameraPos();
	Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
	m_v3Pos = Vector3(v2Cursor.x+v2CamPos.x, v2Cursor.y+v2CamPos.y, m_v3Pos.z);

	if (m_provider->GetInput()->GetKeyState(GSK_SHIFT) == GSKS_DOWN && m_currentEntity->GetCurrentSize() != Vector2(0,0))
	{
		int sizeX = (int)m_currentEntity->GetCurrentSize().x;
		int sizeY = (int)m_currentEntity->GetCurrentSize().y;

		const float angle = m_currentEntity->GetAngle();
		if (angle != 0.0f && m_currentEntity->GetType() == ETH_HORIZONTAL)
		{
			const float fSizeX = (float)sizeX;
			const float fSizeY = (float)sizeY;
			const float a  = DegreeToRadian(90.0f - angle);
			const float b  = DegreeToRadian(90.0f - (90.0f - angle));
			const float e  = DegreeToRadian(90.0f - (90.0f - angle));
			const float bl = DegreeToRadian(90.0f - (90.0f - (90.0f - angle)));
			const float A  = sinf(a)/(1.0f/Max(1.0f, fSizeX));
			const float B  = sinf(b)/(1.0f/Max(1.0f, fSizeX));
			const float F  = 1/(sinf(bl)/B);
			const float E  = sinf(e)/(1.0f/F);
			const float Bl = sinf(bl)/(1.0f/Max(1.0f, fSizeY));
			sizeX = int((A+E)/2.0f);

			if (fSizeX < fSizeY)
				sizeY = int(B);
			else
				sizeY = int(Bl);
		}

		Vector2 v2PosFix;
		if (m_currentEntity->GetType() == ETH_VERTICAL)
		{
			sizeY = (int)m_currentEntity->GetCurrentSize().x;
			v2PosFix.x = float((int)Abs(m_v3Pos.x)%Max(1, (sizeX/4)));
			v2PosFix.y = float((int)Abs(m_v3Pos.y)%Max(1, (sizeY/4)));
		}
		else
		{
			//sizeY = (int)m_currentEntity->GetCurrentSize().y;
			v2PosFix.x = float((int)Abs(m_v3Pos.x)%Max(1, (sizeX/2)));
			v2PosFix.y = float((int)Abs(m_v3Pos.y)%Max(1, (sizeY/2)));
		}

		const float division = (m_currentEntity->GetType() == ETH_VERTICAL) ? 4.0f : 2.0f;

		if (m_v3Pos.x > 0)
		{
			m_v3Pos.x -= v2PosFix.x;
			m_v3Pos.x += (float)sizeX/division;
		}
		else
		{
			m_v3Pos.x += v2PosFix.x;
			m_v3Pos.x -= (float)sizeX/division;
		}

		if (m_v3Pos.y > 0)
		{
			m_v3Pos.y -= v2PosFix.y;
			m_v3Pos.y += (float)sizeY/division;
		}
		else
		{
			m_v3Pos.y += v2PosFix.y;
			m_v3Pos.y -= (float)sizeY/division;
		}
		DrawGrid();
	}
	else
	{
		if (m_currentEntity->GetType() != ETH_VERTICAL)
		{
			const Vector2 v2TipPos(0.0f, m_provider->GetVideo()->GetScreenSizeF().y-m_menuSize-m_menuSize-_ENTITY_SELECTION_BAR_HEIGHT);
			ShadowPrint(v2TipPos, L"You can hold SHIFT to align the entity like in a tile map", GS_COLOR(255,255,255,255));
		}
	}

	// Controls the angle
	if (m_currentEntity->GetType() != ETH_VERTICAL && allowSwapWheel)
	{
		if (m_provider->GetInput()->GetKeyState(GSK_Q) == GSKS_HIT)
			m_currentEntity->AddToAngle(5);
		if (m_provider->GetInput()->GetKeyState(GSK_W) == GSKS_HIT)
			m_currentEntity->AddToAngle(-5);
	}
	else
	{
		m_currentEntity->SetAngle(0.0f);
	}

	// Min and max screen depth for the temporary current entity
	const float maxDepth = m_provider->GetVideo()->GetScreenSizeF().y;
	const float minDepth =-m_provider->GetVideo()->GetScreenSizeF().y;

	// draw a shadow preview for the current entity
	ETHLight shadowLight(true);
	shadowLight.castShadows = true; shadowLight.range = _ETH_DEFAULT_SHADOW_RANGE * 10;
	shadowLight.pos = m_currentEntity->GetPosition()+Vector3(100, 100, 0); shadowLight.color = Vector3(0.5f, 0.5f, 0.5f);
	if (m_currentEntity->GetEntityName() != L"")
	{
		if (m_provider->GetShaderManager()->BeginShadowPass(m_currentEntity, &shadowLight, maxDepth, minDepth))
		{
			m_currentEntity->DrawShadow(maxDepth, minDepth, *m_pScene->GetSceneProperties(), shadowLight, 0, false, false);
			m_provider->GetShaderManager()->EndShadowPass();
		}
	}

	const bool zBuffer = m_provider->GetVideo()->GetZBuffer();
	m_provider->GetVideo()->SetZBuffer(false);
	// draws the current entity ambient pass
	if (m_provider->GetShaderManager()->BeginAmbientPass(m_currentEntity, m_provider->GetVideo()->GetScreenSizeF().y, -m_provider->GetVideo()->GetScreenSizeF().y))
	{
		if (!m_currentEntity->IsInvisible())
		{
			m_currentEntity->DrawAmbientPass(maxDepth, minDepth, false, *m_pScene->GetSceneProperties());
		}
		else
		{
			if (m_currentEntity->IsCollidable())
			{
				m_currentEntity->DrawCollisionBox(true, m_outline, GS_WHITE, m_sceneProps.zAxisDirection);
				m_currentEntity->DrawCollisionBox(false, m_outline, GS_WHITE, m_sceneProps.zAxisDirection);
			}
			else
			{
				m_invisible->Draw(m_currentEntity->GetPositionXY());
			}
		}
		m_provider->GetShaderManager()->EndAmbientPass();
	}
	m_provider->GetVideo()->SetZBuffer(zBuffer);

	// draws the light symbol to show that this entity has light
	if (m_entityFiles[m_currentEntityIdx]->light)
	{
		Vector3 v3Light = m_v3Pos + m_entityFiles[m_currentEntityIdx]->light->pos;
		m_lSprite->Draw(Vector2(v3Light.x, v3Light.y - v3Light.z),
						ConvertToDW(m_entityFiles[m_currentEntityIdx]->light->color));
	}

	// draws the particle symbols to show that the entity has particles
	for (std::size_t t=0; t<m_entityFiles[m_currentEntityIdx]->particleSystems.size(); t++)
	{
		if (m_entityFiles[m_currentEntityIdx]->particleSystems[t]->nParticles > 0)
		{
			const Vector3 v3Part = m_v3Pos+m_entityFiles[m_currentEntityIdx]->particleSystems[t]->v3StartPoint;
			m_pSprite->Draw(Vector2(v3Part.x, v3Part.y-v3Part.z),
							ConvertToDW(m_entityFiles[m_currentEntityIdx]->particleSystems[t]->v4Color0));
		}
	}

	// if the current entity has lights... lit the scene this time
	if (m_currentEntity->HasLightSource())
	{
		ETHLight light = *m_currentEntity->GetLight();
		light.pos.x += v2Cursor.x + v2CamPos.x;
		light.pos.y += v2Cursor.y + v2CamPos.y;
		light.staticLight = false;
		m_pScene->AddLight(light);
	}

	// if the current entity has sound effects attached to it's particle systems, show the sound wave bitmap
	if (m_currentEntity->HasSoundEffect())
	{
		m_soundWave->Draw(m_currentEntity->GetPositionXY(), ARGB(150, 255, 255, 255));
	}
}

void SceneEditor::UpdateInternalData()
{
	if (m_updateLights && m_pScene->AreLightmapsEnabled())
	{
		m_pScene->GenerateLightmaps(m_genLightmapForThisOneOnly);
		m_updateLights = false;
		m_genLightmapForThisOneOnly =-1;
	}
}

void SceneEditor::EntityPlacer()
{
	if (m_entityFiles.empty())
		return;

	if (m_provider->GetInput()->GetLeftClickState() == GSKS_HIT)
	{
		string currentProjectPath = GetCurrentProjectPath(true);
		ETHRenderEntity* entity = new ETHRenderEntity(m_provider, *m_currentEntity->GetProperties(), m_currentEntity->GetAngle(), 1.0f);
		entity->SetOrphanPosition(m_v3Pos);
		entity->SetAngle(m_currentEntity->GetAngle());
		entity->SetColor(ETH_WHITE_V4);

		const int id = m_pScene->AddEntity(entity);
		if (m_pScene->CountLights() && (m_currentEntity->IsStatic()))
		{
			m_updateLights = true;

			// if possible, generate the lightmap for only this one
			if (!entity->HasShadow() && !entity->HasLightSource())
			{
				m_genLightmapForThisOneOnly = id;
			}
			else
			{
				ShowLightmapMessage();
			}
		}
	}

	// Show entity info
	wstringstream ss;
	ss << ETHGlobal::Vector3ToString(m_currentEntity->GetPosition()) << endl;
	const float angle = m_currentEntity->GetAngle();
	if (angle != 0.0f)
	{
		char szAngle[32];
		_ETH_SAFE_sprintf(szAngle, "%.0f", angle);
		ss << szAngle;
	}
	ShadowPrint(m_currentEntity->GetScreenRect(*m_pScene->GetSceneProperties()).v2Max, ss.str().c_str(), GS_COLOR(100, 255, 255, 255));
}

void SceneEditor::DrawEntitySelectionGrid(SpritePtr pNextAppButton)
{
	const Vector2 v2Cursor = m_provider->GetInput()->GetCursorPositionF(m_provider->GetVideo());
	const Vector2 v2Screen = m_provider->GetVideo()->GetScreenSizeF();
	const Vector2 v2HalfTile(_ENTITY_SELECTION_BAR_HEIGHT/2, _ENTITY_SELECTION_BAR_HEIGHT/2);
	//const Vector2 v2Tile(_ENTITY_SELECTION_BAR_HEIGHT, _ENTITY_SELECTION_BAR_HEIGHT);
	static const float barHeight = _ENTITY_SELECTION_BAR_HEIGHT;
	const float barWidth = v2Screen.x-pNextAppButton->GetBitmapSizeF().x;
	const float barPosY = v2Screen.y-barHeight;
	const GSGUI_STYLE *pStyle = m_fileMenu.GetGUIStyle();
	m_provider->GetVideo()->DrawRectangle(Vector2(0, barPosY), Vector2(barWidth, barHeight),
		0x00000000, 0x00000000, pStyle->inactive_bottom, pStyle->inactive_bottom, 0.0f);

	if (m_entityFiles.empty())
		return;

	const bool zBuffer = m_provider->GetVideo()->GetZBuffer();
	const bool roundUp = m_provider->GetVideo()->IsRoundingUpPosition();
	m_provider->GetVideo()->SetZBuffer(false);
	m_provider->GetVideo()->RoundUpPosition(true);

	const float fullLengthSize = _ENTITY_SELECTION_BAR_HEIGHT*static_cast<float>(m_entityFiles.size());
	float globalOffset = 0.0f;

	// adjust the horizontal offset
	if (fullLengthSize > barWidth)
		globalOffset = (-(Clamp(v2Cursor.x, 0.0f, barWidth)/barWidth)*(fullLengthSize-barWidth));

	float offset = 0.0f;
	float heightOffset = _ENTITY_SELECTION_BAR_HEIGHT;

	// adjust the vertical offset
	if (v2Cursor.y >= v2Screen.y-_ENTITY_SELECTION_BAR_PROXIMITY)
	{
		float cursorProximityFactor = (v2Cursor.y-(v2Screen.y-_ENTITY_SELECTION_BAR_PROXIMITY-_ENTITY_SELECTION_BAR_HEIGHT))/_ENTITY_SELECTION_BAR_PROXIMITY;
		cursorProximityFactor = Max(0.0f, Min(cursorProximityFactor, 1.0f));
		heightOffset *= (1-cursorProximityFactor);
	}
	else
	{
		return;
	}

	wstring textToDraw;
	const Vector2 v2Cam = m_provider->GetVideo()->GetCameraPos();
	m_provider->GetVideo()->SetCameraPos(Vector2(0,0));
	for (std::size_t t = 0; t < m_entityFiles.size(); t++)
	{
		SpritePtr pSprite = m_provider->GetGraphicResourceManager()->GetPointer(
			m_provider->GetVideo(), 
			utf8::c(m_entityFiles[t]->spriteFile).wc_str(), utf8::c(GetCurrentProjectPath(true)).wc_str(), 
			ETHDirectories::GetEntityPath(), false
		);

		if (!m_entityFiles[t]->spriteFile.empty() && !pSprite)
		{
			m_provider->GetVideo()->Message(str_type::string(GS_L("Removing entity from list: ")) + m_entityFiles[t]->entityName, GSMT_WARNING);
			m_entityFiles.erase(m_entityFiles.begin() + t);
			t--;
			continue;
		}

		const Vector2 v2Pos = Vector2(offset+globalOffset, barPosY+heightOffset)+v2HalfTile;

		// skip it if it's not yet visible
		if (v2Pos.x+_ENTITY_SELECTION_BAR_HEIGHT < 0)
		{
			offset += _ENTITY_SELECTION_BAR_HEIGHT;
			continue;
		}

		float bias = 1.0f;
		if (pSprite)
		{
			const Vector2 v2Size = pSprite->GetFrameSize();
			const float largestSize = Max(v2Size.x, v2Size.y);
			Vector2 v2NewSize = v2Size;
			if (v2Size.x > _ENTITY_SELECTION_BAR_HEIGHT || v2Size.y > _ENTITY_SELECTION_BAR_HEIGHT)
			{
				bias = (largestSize/_ENTITY_SELECTION_BAR_HEIGHT);
				v2NewSize = v2Size/bias;
			}
			const Vector2 v2Origin = pSprite->GetOrigin();

			pSprite->SetOrigin(GSEO_CENTER);
			//const unsigned int nFrame = pSprite->GetCurrentRect();
			if (m_entityFiles[t]->spriteCut.x > 1 || m_entityFiles[t]->spriteCut.y > 1)
			{
				pSprite->SetupSpriteRects(m_entityFiles[t]->spriteCut.x, m_entityFiles[t]->spriteCut.y);
				pSprite->SetRect(m_entityFiles[t]->startFrame);
			}
			pSprite->DrawShaped(v2Pos, v2NewSize,
								GS_WHITE, GS_WHITE, GS_WHITE, GS_WHITE, 0.0f);
			//pSprite->SetRect(nFrame);

			pSprite->SetOrigin(v2Origin);
		}
		else if (!m_entityFiles[t]->light) // if it has no light source and no particles, draw the "invisible" symbol
		{
			bool hasParticles = false;
			for (std::size_t p = 0; p < m_entityFiles[t]->particleSystems.size(); p++)
			{
				if (m_entityFiles[t]->particleSystems[p]->nParticles > 0)
					hasParticles = true;
			}
			if (!hasParticles)
				m_invisible->Draw(v2Pos);
		}

		// if it has sound effect, show it
		for (std::size_t p = 0; p < m_entityFiles[t]->particleSystems.size(); p++)
		{
			if (m_entityFiles[t]->particleSystems[p]->soundFXFile.length() > 0)
			{
				const Vector2 v2Size = m_soundWave->GetBitmapSizeF();
				const float largestSize = Max(v2Size.x, v2Size.y);
				const float bias = (largestSize / _ENTITY_SELECTION_BAR_HEIGHT);
				const Vector2 v2NewSize = v2Size / bias;
				m_soundWave->DrawShaped(v2Pos, v2NewSize, GS_WHITE, GS_WHITE, GS_WHITE, GS_WHITE, 0.0f);
				break;
			}
		}

		// if it has particle systems, show the P symbol
		for (std::size_t p = 0; p < m_entityFiles[t]->particleSystems.size(); p++)
		{
			if (m_entityFiles[t]->particleSystems[p]->nParticles > 0)
			{
				Vector2 v2SymbolPos = Vector2(
					m_entityFiles[t]->particleSystems[p]->v3StartPoint.x,
					m_entityFiles[t]->particleSystems[p]->v3StartPoint.y - m_entityFiles[t]->particleSystems[p]->v3StartPoint.z) / bias;
				v2SymbolPos.x = Clamp(v2SymbolPos.x,-_ENTITY_SELECTION_BAR_HEIGHT/2, _ENTITY_SELECTION_BAR_HEIGHT/2);
				v2SymbolPos.y = Clamp(v2SymbolPos.y,-_ENTITY_SELECTION_BAR_HEIGHT/2, _ENTITY_SELECTION_BAR_HEIGHT/2);
				const Vector2 v2PPos = v2Pos+v2SymbolPos;
				m_pSprite->Draw(v2PPos, ConvertToDW(m_entityFiles[t]->particleSystems[p]->v4Color0));
			}
		}

		// if it has light, show the light symbol
		if (m_entityFiles[t]->light)
		{
			Vector2 v2SymbolPos = Vector2(m_entityFiles[t]->light->pos.x, m_entityFiles[t]->light->pos.y - m_entityFiles[t]->light->pos.z) / bias;
			v2SymbolPos.x = Clamp(v2SymbolPos.x,-_ENTITY_SELECTION_BAR_HEIGHT/2, _ENTITY_SELECTION_BAR_HEIGHT/2);
			v2SymbolPos.y = Clamp(v2SymbolPos.y,-_ENTITY_SELECTION_BAR_HEIGHT/2, _ENTITY_SELECTION_BAR_HEIGHT/2);
			const Vector2 v2LightPos = v2Pos + v2SymbolPos;
			m_lSprite->Draw(v2LightPos, ConvertToDW(m_entityFiles[t]->light->color));
		}

		// highlight the 'mouseover' entity
		if (ETHGlobal::PointInRect(v2Cursor, v2Pos, Vector2(_ENTITY_SELECTION_BAR_HEIGHT,_ENTITY_SELECTION_BAR_HEIGHT)))
		{
			const GS_COLOR dwColor(55,255,255,255);
			m_outline->SetOrigin(GSEO_CENTER);
			m_outline->DrawShaped(v2Pos, Vector2(_ENTITY_SELECTION_BAR_HEIGHT,_ENTITY_SELECTION_BAR_HEIGHT),
								  dwColor, dwColor, dwColor, dwColor);

			// if the user clicked, select this one
			if (m_provider->GetInput()->GetKeyState(GSK_LMOUSE) == GSKS_HIT)
			{
				m_v3Pos.z = 0.0f;
				m_currentEntityIdx = t;
				SetPlacingMode();
			}

			textToDraw = utf8::c(m_entityFiles[t]->entityName).wc_str();
			textToDraw += L" {";
			textToDraw += (m_entityFiles[t]->staticEntity) ? L"static/" : L"dynamic/";
			textToDraw += ETHGlobal::wcsEntityType[m_entityFiles[t]->type];
			textToDraw += L"}";
		}

		// highlight the current entity
		if (m_currentEntityIdx == t)
		{
			const GS_COLOR dwColor(127,255,255,255);
			m_outline->SetOrigin(GSEO_CENTER);
			m_outline->DrawShaped(v2Pos, Vector2(_ENTITY_SELECTION_BAR_HEIGHT,_ENTITY_SELECTION_BAR_HEIGHT),
								  dwColor, dwColor, dwColor, dwColor);
		}

		// stop drawing if it reaches the end
		if (v2Pos.x > barWidth)
		{
			break;
		}

		offset += _ENTITY_SELECTION_BAR_HEIGHT;
	}

	if (textToDraw != L"")
	{
		const Vector2 v2TextPos(v2Cursor-Vector2(0, m_menuSize));
		const GS_COLOR dwLeft = ARGB(155,0,0,0);
		const GS_COLOR dwRight = ARGB(155,0,0,0);
		Vector2 boxSize = m_provider->GetVideo()->ComputeTextBoxSize(L"Verdana14_shadow.fnt", textToDraw.c_str());
		m_provider->GetVideo()->DrawRectangle(v2TextPos, boxSize, dwLeft, dwRight, dwLeft, dwRight);
		ShadowPrint(v2TextPos, textToDraw.c_str(), GS_COLOR(255,255,255,255));
	}
	m_provider->GetVideo()->SetCameraPos(v2Cam);

	m_provider->GetVideo()->RoundUpPosition(roundUp);
	m_provider->GetVideo()->SetZBuffer(zBuffer);
}

void SceneEditor::LoopThroughEntityList()
{
	ETHEntityArray entityArray;
	m_pScene->GetBucketManager().GetVisibleEntities(entityArray);

	for (unsigned int t=0; t<entityArray.size(); t++)
	{
		const ETHRenderEntity *pEntity = static_cast<ETHRenderEntity*>(entityArray[t]);
		const Vector3 v3EntityPos(pEntity->GetPosition());

		// draw light symbol where we have invisible light sources
		if (pEntity->IsInvisible() && !pEntity->HasHalo())
		{
			if (pEntity->HasLightSource())
			{
				const Vector3 v3LightPos(pEntity->GetLightRelativePosition());
				m_lSprite->Draw(Vector2(v3EntityPos.x + v3LightPos.x, v3EntityPos.y + v3LightPos.y - v3EntityPos.z - v3LightPos.z),
					ConvertToDW(pEntity->GetLightColor()));
			}
			else if (pEntity->IsCollidable())
			{
				pEntity->DrawCollisionBox(true, m_outline, GS_WHITE, m_pScene->GetZAxisDirection());
				pEntity->DrawCollisionBox(false, m_outline, GS_WHITE, m_pScene->GetZAxisDirection());
			}
		}

		// draw custom data
		if (pEntity->HasCustomData() && m_panel.GetButtonStatus(_S_SHOW_CUSTOM_DATA))
		{
			m_customDataEditor.ShowInScreenCustomData(pEntity, this);
		}
	}
}

void SceneEditor::UpdateEntities()
{
	ETHEntityArray entityArray;
	m_pScene->GetBucketManager().GetEntityArray(entityArray);
	const string currentProjectPath = GetCurrentProjectPath(true);
	const unsigned int size = entityArray.size();
	for (unsigned int t=0; t<size; t++)
	{
		const int idx = GetEntityByName(utf8::c(entityArray[t]->GetEntityName()).str());
		if (idx >= 0)
		{
			entityArray[t]->Refresh(*m_entityFiles[idx].get());
		}
	}
}

int SceneEditor::GetEntityByName(const string &name)
{
	for (std::size_t t = 0; t < m_entityFiles.size(); t++)
	{
		if (utf8::c(m_entityFiles[t]->entityName).str() == name)
		{
			return t;
		}
	}
	return -1;
}

void SceneEditor::CopySelectedToClipboard()
{
	if (!m_pSelected)
		return;

	const Vector2 v2Size = m_pSelected->GetCurrentSize();
	m_clipBoard = boost::shared_ptr<ETHRenderEntity>(new ETHRenderEntity(m_provider, *m_pSelected->GetProperties(), m_pSelected->GetAngle(), 1.0f));
	m_clipBoard->SetOrphanPosition(m_pSelected->GetPosition() + Vector3(v2Size.x, v2Size.y, 0));
	m_clipBoard->SetAngle(m_pSelected->GetAngle());
	m_clipBoard->SetFrame(m_pSelected->GetFrame());
	m_clipBoard->SetColor(m_pSelected->GetColorARGB());
}

void SceneEditor::PasteFromClipboard()
{
	if (!m_clipBoard)
		return;

	ETHRenderEntity* entity = new ETHRenderEntity(m_provider);
	*entity = *m_clipBoard.get();
	m_pScene->AddEntity(entity);
}

void SceneEditor::RenderScene()
{
	m_pScene->SetBorderBucketsDrawing(false);
	const unsigned long lastFrameElapsedTimeMS = ComputeElapsedTime(m_provider->GetVideo());
	m_pScene->Update(lastFrameElapsedTimeMS);
	m_pScene->UpdateTemporary(lastFrameElapsedTimeMS);
	m_pScene->RenderScene(false, lastFrameElapsedTimeMS, m_outline, m_invisible);
}

bool SceneEditor::SaveAs()
{
	char filter[] = "Ethanon Scene files (*.esc)\0*.esc\0\0";
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	if (SaveForm(filter, utf8::c(ETHDirectories::GetScenePath()).c_str(), path, file, GetCurrentProjectPath(true).c_str()))
	{
		string sOut;
		AddExtension(path, ".esc", sOut);
		if (m_pScene->SaveToFile(utf8::c(sOut).wstr()))
		{
			SetCurrentFile(sOut.c_str());
			CreateFileUpdateDetector(utf8::c(GetCurrentFile(true)).wstr());
		}
	}
	return true;
}

bool SceneEditor::Save()
{
	m_pScene->SaveToFile(utf8::c(GetCurrentFile(true)).wstr());
	CreateFileUpdateDetector(utf8::c(GetCurrentFile(true)).wstr());
	return true;
}

bool SceneEditor::Open()
{
	char filter[] = "Ethanon Scene files (*.esc)\0*.esc\0\0";
	char path[___OUTPUT_LENGTH], file[___OUTPUT_LENGTH];
	if (OpenForm(filter, utf8::c(ETHDirectories::GetScenePath()).c_str(), path, file, GetCurrentProjectPath(true).c_str()))
	{
		OpenByFilename(path);
	}
	return true;
}

void SceneEditor::OpenByFilename(const string& filename)
{
	RebuildScene(utf8::c(filename).wstr());
	if (m_pScene->GetNumEntities())
	{
		m_sceneProps = *m_pScene->GetSceneProperties();
		SetCurrentFile(filename.c_str());
		ResetForms();
		if (m_panel.GetButtonStatus(_S_USE_STATIC_LIGHTMAPS))
		{
			m_updateLights = true;
		}
		//m_currentEntityIdx = 0;
		m_provider->GetVideo()->SetCameraPos(Vector2(0,0));
		CreateFileUpdateDetector(utf8::c(filename).wstr());
	}
}

void SceneEditor::ShowLightmapMessage()
{
	ShadowPrint(m_provider->GetVideo()->GetScreenSizeF()/2-Vector2(400,0), L"Generating lightmap. Please wait...", L"Verdana30_shadow.fnt", GS_WHITE);
}

void SceneEditor::CentralizeShortcut()
{
	if (m_provider->GetInput()->GetKeyState(GSK_CTRL) == GSKS_DOWN && m_provider->GetInput()->GetKeyState(GSK_SPACE) == GSKS_HIT)
	{
		m_v2CamPos = Vector2(0,0);
	}
}

void SceneEditor::ReloadFiles()
{
	m_entityFiles.clear();

	if (GetCurrentProjectPath(false) == "")
	{
		return;
	}

	Platform::FileListing entityFiles;
	string currentProjectPath = GetCurrentProjectPath(true) + utf8::c(ETHDirectories::GetEntityPath()).c_str();

	entityFiles.ListDirectoryFiles(utf8::c(currentProjectPath).wc_str(), L"ent");
	const int nFiles = entityFiles.GetNumFiles();
	if (nFiles >= 1)
	{
		m_entityFiles.resize(nFiles);
		for (int t=0; t<nFiles; t++)
		{
			Platform::FileListing::FILE_NAME file;
			entityFiles.GetFileName(t, file);
			wstring full;
			file.GetFullFileName(full);
			string sFull = utf8::c(full).str();

			cout << "(Scene editor)Entity found: " <<  utf8::c(file.file).str() << " - Loading...";

			if (m_entityFiles[t] = boost::shared_ptr<ETHEntityProperties>(
				new ETHEntityProperties(utf8::c(sFull).wstr(), m_provider->GetVideo()->GetFileManager())))
			{
				cout << " loaded";
			}
			else
			{
				cerr << " invalid file.";
			}

			cout << endl;
		}
	}
	else
	{
		wcerr << L"The editor couldn't find any *.ent files. Make sure there are valid files at the "
			 << ETHDirectories::GetEntityPath() << L" folder\n";
	}
}

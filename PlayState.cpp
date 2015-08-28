#include "PlayState.h"


CPlayState::CPlayState(char *pathmap, MGraphic* gra, LPDIRECT3DDEVICE9 d3ddev, MKeyboard* input, Camera* cam, int id)
{
	_isStopScreen = false;
		_isPause =  false;
		cam->LoadCameraPath(id);
		this->_camera = cam;
		this->_graphic = gra;
		_graphic->GetCam(cam);
		_graphic->SetDevice(d3ddev);
		content = new MContent(gra->GetDevice());
		this->SetInput(input);

		_rockman = new Megaman();
		_rockman->Initlize();
		_changingScreen = 0;
		_pointAfterDoor = D3DXVECTOR2(0, 0);
		_pointBeforeDoor =D3DXVECTOR2(0, 0);
		_isBossDied = false;
		
		//Load map - Load quadtree
		tree = new CQuadTree();
		tree->LoadMap(id);
		// Khởi động trạng thái
		_playState = PlayState::READY;
		_deltaClearPoint = 0;
		_strClearPoint = "";
		_strBonus = "";
		_deltaTime = 0;
		_typeID = ID_SCREEN_PLAY;
		_prepareForBoss = 0;
		_shakePointRand = D3DXVECTOR2(0.0f, 0.0f);
		_defaultStringColor = D3DCOLOR_XRGB(255, 255, 255);
		D3DXVECTOR2 pos = _camera->endMap;;
		_camera->_pos = pos;
		_camera->_pos.x = pos.x - 200;
		_rockman->SetPos(D3DXVECTOR2(pos.x - 200 + SCREEN_WIDTH / 2,pos.y));
/*		
		_rockman = new _rockman();
		_rockman->Initlize();
*/
		_map = new Map();
		_map->Init(content);

		tileManager = new CTile(gra);
		tileManager->LoadTile(pathmap, id);

		bubble = new  CEnemyBubble(1, 1, ResourceManager::GetSprite(ID_SPRITE_ENEMY_BUBBLE_BLUE), ResourceManager::GetSprite(ID_SPRITE_EXPLODING_EFFECT_BASE),
			D3DXVECTOR2(50.f / 1000.0f, 0.0f / 1000.0f),
			D3DXVECTOR2(200, 100), DAME_ENEMY_BUBBLE, BLOOD_ENEMY_BUBBLE, 2.5* SCORE_DEFAULT);
		bubble->_size = D3DXVECTOR2(32, 32);
		bubble->Initlize();
		//obj->SetCollideRegion(posXCollide, posYCollide, widthCollide, heightCollide);
	listBubble.push_back(bubble);
		

		/*_rockman->_position = D3DXVECTOR2(100, 200);
		_rockman->_behave = STAND;*/
		_input->Active();

}

void CPlayState::UpdateKeyboard(MKeyboard* input)
{
	
	if (_playState == PlayState::PLAYING&& _rockman->_behave != Behave::DYING && _rockman->_behave != Behave::REAL_DIE&& !_rockman->IsRequireStopScreen() && _changingScreen == 0)
	{
		if (input->IsKeyDown(DIK_SPACE))
		{
			if (_isPause)
			{
				_isPause = false;
				_rockman->ReSetSKill();
				ResourceManager::ReplaySound();
			}
			else{
				ResourceManager::MuteSound();
				ResourceManager::PlayASound(ID_EFFECT_PAUSE);
				_isPause = true;
			}

		}
		if (input->IsKeyDown(DIK_RETURN))
		{
			if (!_isPause)
			{
				_rockman->ReSetSKill();
				_isChosingSkill = true;
				CScreenManager::GetInstance()->ShowPopupScreen(new CPopup(_rockman, this->_graphic));
				ResourceManager::PlayASound(ID_EFFECT_POPUP_APPEAR);
			}
		}
	}
}

void CPlayState::Update(CTimer* gameTime)
{
	if (!_isPause)
	{
		///Nếu của sổ chọn skill đang mở
		if (_isChosingSkill)
		{
			_isChosingSkill = false;

			int countPowerEnegy = _powerEnegies.size();
			for (int i = 0; i < countPowerEnegy; i++)
				switch (_powerEnegies[i]->_typeID)
			{
				case ID_BAR_WEAPONS_BOOM:
				case ID_BAR_WEAPONS_CUT:
				case ID_BAR_WEAPONS_GUTS:
					_powerEnegies.erase(_powerEnegies.begin() + i);
					countPowerEnegy -= 1;
					i -= 1;
					break;
			}
			switch (_rockman->GetCurrentSkill())
			{
			case Skill::CUT:
				_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_WEAPONS_CUT, _rockman, ResourceManager::GetSprite(ID_SPRITE_BAR_CUT_VERTICAL), D3DXVECTOR2(19, -37), 0, WEASPON_DEFAULT, WEASPON_DEFAULT));
				break;
			case Skill::GUTS:
				_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_WEAPONS_GUTS, _rockman, ResourceManager::GetSprite(ID_SPRITE_BAR_GUTS_VERTICAL), D3DXVECTOR2(19, -37), 0, WEASPON_DEFAULT, WEASPON_DEFAULT));
				break;
			case Skill::BOOM:
				_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_WEAPONS_BOOM, _rockman, ResourceManager::GetSprite(ID_SPRITE_BAR_BOOM_VERTICAL), D3DXVECTOR2(19, -37), 0, WEASPON_DEFAULT, WEASPON_DEFAULT));
				break;
			}
		}
		if (_isStopScreen)
		{
			_deltaTimeStopScreen += gameTime->GetTime();
			if (_deltaTimeStopScreen >= 1000)
			{
				_isStopScreen = false;
				_rockman->_isRequireStopScreen = false;
			}
			else return;
		}


	
	
	
	//_camera->Update(_rockman->_position);
	tree->_listObjectOnScreen.clear();
	tree->ClipCamera(tree->_nodeRoot, _camera->getViewPort());
	

	_groundObjs.clear();
	_elevators.clear();
	//_enemies.clear();
	_items.clear();
	_listDoor.clear();

	// lay danh sach cac doi tuong chia ra theo tung loai
	for(map<int,CGameObject*>::iterator i = tree->_listObjectOnScreen.begin();i != tree->_listObjectOnScreen.end();++i)
	{
		
		switch ((*i).second->_typeID)
		{
		case ID_ROCK_IN_GUT_STAGE:
		case ID_ROCK:
			if (((CRock*)(*i).second)->IsGot())
				_groundObjs.push_back((*i).second);
				break;
		case  ID_BLOCK_TROUBLE_OF_ELEVATOR:
		case ID_BLOCK:
		case ID_STAIR:
		case ID_FALLING_POINT:
		case ID_DIE_ARROW:
			_groundObjs.push_back((*i).second);
			break;
		case ID_DOOR1_CUTMAN:
		case ID_DOOR2_CUTMAN:
		case ID_DOOR1_GUTSMAN:
		case ID_DOOR2_GUTSMAN:
		case ID_DOOR1_BOOMMAN:
		case ID_DOOR2_BOOMMAN:
			_door = ((CDoor*)((*i).second));
			_listDoor.push_back(((CDoor*)((*i).second)));
			break;
		case ID_ITEM_BLOOD_BIG:
		case ID_ITEM_BLOOD_SMALL:
		case ID_ITEM_LIFE:
		case ID_ITEM_MANA_BIG:
		case ID_ITEM_MANA_SMALL:
			{
				bool existed = false;

				for (int j = 0; j < _items.size(); j++)
				{
					if ((*i).second->_id == _items[j]->_id)
					{
						existed = true;
						break;
					}
				}
				if (!existed)
					_items.push_back(((CItemObject*)((*i).second)->ToValue()));
				break;
			}
		case ID_ELEVATOR_TROUBLE:
		case ID_ELEVATOR:
			if ((*i).second->GetBox().IntersecWith(_camera->getViewPort()) || _playState == PlayState::READY)
			{
				bool existed = false;
				for (int j = 0; j < _elevators.size(); j++)
				{
					if ((*i).second->_id == _elevators[j]->_id)
					{
						existed = true;
						break;
					}
				}
				if (!existed)
					_elevators.push_back((CElevator*)((*i).second));
			}
			break;
		case ID_CUTMAN:
			if (_prepareForBoss == 0)
			{
				_prepareForBoss = 1;
				_deltaTime = 0;
				_spriteIntroBoss = ResourceManager::GetSprite(ID_SPRITE_ROOM_CUT_STAGE);
			}
		case ID_BOOMMAN:
			if (_prepareForBoss == 0)
			{
				_prepareForBoss = 1;
				_deltaTime = 0;
				_spriteIntroBoss = ResourceManager::GetSprite(ID_SPRITE_ROOM_BOOM_STAGE);
			}
		case ID_GUTSMAN:
			if (_prepareForBoss == 0)
			{
				_prepareForBoss = 1;
				_deltaTime = 0;
				_spriteIntroBoss = ResourceManager::GetSprite(ID_SPRITE_ROOM_GUTS_STAGE);
			}
			if (_isBossDied)
				break;
			if (_prepareForBoss == 2 && _camera->_pos == _camera->_positionBossRoom )
			{
				bool exited = false;

				for (int j = 0; j < _enemies.size(); j++)
				{
					if ((*i).second->_id == _enemies[j]->_id)
					{
						exited = true;
						break;
					}
				}
				if (!exited)
					_enemies.push_back(((CEnemy*)(*i).second->ToValue()));

			}
			break;
		default:
			if ((*i).second->GetBox().IntersecWith(_camera->getViewPort()) || _playState == PlayState::READY)
			{
			bool existed = false;

			for (int j = 0; j < _enemies.size(); j++)
			{
				if((*i).second->_typeID == 5000)
					int a = 0;
				if ((*i).second->_id == _enemies[j]->_id)
				{
					existed = true;
					break;
				}
			}
			if (!existed)
				_enemies.push_back(((CEnemy*)(*i).second->ToValue()));
			}
			break;
		}
	}


		if (_elevators.size() == 0)
			ResourceManager::StopSound(ID_EFFECT_ELEVATOR_RUNNING);
		else
			ResourceManager::PlayASound(ID_EFFECT_ELEVATOR_RUNNING);

		if (_changingScreen == 2)
			_changingScreen = 0;

	if (_playState == PlayState::READY)
	{
#pragma region Xử lý khởi động màn chơi PlayState::READY
		_deltaTime += gameTime->GetTime();
		if (_deltaTime >= 1000)
		{
			_rockman->Update(gameTime, _input);
			//_camera->Update(_rockman->_position);
			CDirection normalX, normalY;
			float collideTime;

			for (int i = 0; i < _groundObjs.size(); i++)		// Kiểm tra va chạm với các khối tường, gạch đá
			{
				collideTime = CheckCollision(_rockman, _groundObjs[i], normalX, normalY, gameTime->GetTime());
				if (collideTime < gameTime->GetTime())
					_rockman->OnCollideWith(_groundObjs[i], normalX, normalY, collideTime);
			}

			// Đảm bảo rockman đã chuyển sang chế độ play,
			if (!_rockman->_isTheFirstTime)
			{
				_playState = PlayState::PLAYING;
				_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_BLOOD_ROCKMAN, _rockman, ResourceManager::GetSprite(ID_SPRITE_BAR_ROCKMAN_VERTICAL), D3DXVECTOR2(27, -37), 0, BLOOD_DEFAULT, BLOOD_DEFAULT));
				return;
			}
		}
#pragma endregion
	}
	else if (_playState == PlayState::PLAYING)
	{
#pragma region Xử lý khi vào phòng boss

		if (_prepareForBoss == 1)
		{
			_deltaTime += gameTime->GetTime();
			if (_deltaTime >= 3000)
			{
				_prepareForBoss = 2;
				_deltaTime = 0;
				_playState = PlayState::PLAYING;
			}
		}

#pragma endregion

#pragma region Xử lý chạy game
		if (_rockman->IsDied())
		{
			_playState = PlayState::GAMEOVER;
			_bulletsRockman.clear();
			ResourceManager::PlayASound(ID_EFFECT_ROCKMAN_DIE);
			ResourceManager::StopSound(ID_SOUND_BOSS);
			switch (CGameInfo::GetInstance()->GetLevel())
			{
			case ID_LEVEL_BOOM:
				ResourceManager::StopSound(ID_SOUND_BOMBMAN_STAGE);
				break;
			case ID_LEVEL_CUT:
				ResourceManager::StopSound(ID_SOUND_CUTMAN_STAGE);
				break;
			case ID_LEVEL_GUTS:
				ResourceManager::StopSound(ID_EFFECT_ELEVATOR_RUNNING);
				ResourceManager::StopSound(ID_SOUND_GUTSMAN_STAGE);
				break;
			}
			_deltaTime = 0;						// Bắt đầu đếm thời gian chờ chuyển màn
			return;
		}
		for (int i = 0; i < _listDoor.size(); i++)
		{
			if (_listDoor.at(i) != NULL)
		{
			if (_listDoor.at(i)->_typeID == ID_DOOR1_CUTMAN ||
				_listDoor.at(i)->_typeID == ID_DOOR2_CUTMAN ||
				_listDoor.at(i)->_typeID == ID_DOOR1_GUTSMAN ||
				_listDoor.at(i)->_typeID == ID_DOOR2_GUTSMAN ||
				_listDoor.at(i)->_typeID == ID_DOOR1_BOOMMAN ||
				_listDoor.at(i)->_typeID == ID_DOOR2_BOOMMAN)
			{
				switch (_listDoor.at(i)->_typeID)
				{
				case ID_DOOR1_CUTMAN:
				case ID_DOOR1_GUTSMAN:
				case ID_DOOR1_BOOMMAN:
					_pointBeforeDoor = _listDoor.at(i)->_position - D3DXVECTOR2(SCREEN_WIDTH / 2, 0);
					_pointAfterDoor = _listDoor.at(i)->_position + D3DXVECTOR2(SCREEN_WIDTH / 2 + _listDoor.at(i)->GetBox()._width / 2, 0);
					break;
				case ID_DOOR2_CUTMAN:
				case ID_DOOR2_GUTSMAN:
					_pointBeforeDoor = _listDoor.at(i)->_position - D3DXVECTOR2(SCREEN_WIDTH / 2, 0);
					_pointAfterDoor = _camera->_listPoint.at(_camera->_listPoint.size()-1);
					break;
				case ID_DOOR2_BOOMMAN:
					_pointBeforeDoor = _listDoor.at(i)->_position + D3DXVECTOR2(0, SCREEN_HEIGHT / 2);
					_pointAfterDoor = _camera->_listPoint.at(_camera->_listPoint.size()-1);
					break;
				}
				_pointDoor = _listDoor.at(i)->_position;

				if (_rockman->IsRequireOpenDoor())
				{
					if (_listDoor.at(i)->GetState() == DoorState::WAITING)
					{
						_doorState = -1;
						_listDoor.at(i)->OpenDoor();
						ResourceManager::PlayASound(ID_EFFECT_OPEN_THE_DOOR);
					}
					_listDoor.at(i)->Update(gameTime);

					if (_listDoor.at(i)->GetState() == DoorState::WAITING_FOR_THROUGH&&!_rockman->IsGoingOverDoor())
					{
						_doorState = 0;
						_rockman->ThroughOverDoor(_listDoor.at(i)->_typeID);
						_changingScreen = 1;

						//_enemies.clear();
						//_bulletsEnemy.clear();
						//_bulletsRockman.clear();
						//_items.clear();
						_rockman->_canFire = true;

						if (_listDoor.at(i)->_typeID != ID_DOOR2_BOOMMAN)
						{
							_newScreenPosition = _pointAfterDoor;
							_changeScreenDirection = CDirection::ON_RIGHT;
						}
					}
				}
				else if (_rockman->IsOverDoor() && _listDoor.at(i)->GetState() == DoorState::WAITING_FOR_THROUGH)
				{

					if(_camera->_pos == _camera->_positionBossRoom)
					{
						_rockman->_isInBossRoom= true;
						_rockman->_isOverDoor = true;
					}
					_doorState = 0;
					_listDoor.at(i)->CloseDoor();
					_changingScreen = 0 ;
					_camera->MoveOverDoor();
					ResourceManager::PlayASound(ID_EFFECT_OPEN_THE_DOOR);
					_listDoor.at(i)->Update(gameTime);
				}
			}

			_listDoor.at(i)->Update(gameTime);
		}
		}
		
		// Chặn mọi hoạt động khi cửa hoạt động
		if (_doorState == -1)
			return;

		_rockman->Update(gameTime, _input);
		_camera->Update(_rockman->_position);
		vector<CBullet*> bullets = _rockman->GetBullets();
		for (int i = 0; i < bullets.size(); i++)
			_bulletsRockman.push_back(bullets[i]);

		// Duyệt va chạm giữa Rockman và các đối tượng trên màn hình
		CDirection normalX, normalY;
		float collideTime;

		for (int i = 0; i < _groundObjs.size(); i++)		// Kiểm tra va chạm với các khối tường, gạch đá
		{
			collideTime = CheckCollision(_rockman, _groundObjs[i], normalX, normalY, gameTime->GetTime());
			if (collideTime < gameTime->GetTime())
				_rockman->OnCollideWith(_groundObjs[i], normalX, normalY, collideTime);
		}
		for (int i = 0; i < _listDoor.size(); i++)
		{
			if (_listDoor.at(i) != NULL)
			{
				collideTime = CheckCollision(_rockman, _listDoor.at(i), normalX, normalY, gameTime->GetTime());
				if (collideTime < gameTime->GetTime())
					_rockman->OnCollideWith(_listDoor.at(i), normalX, normalY, collideTime);
			}

		}
		
		for (int i = 0; i < _items.size(); i++)		// Kiểm tra va chạm với các items
		{
			collideTime = CheckCollision(_rockman, _items[i], normalX, normalY, gameTime->GetTime());
			if (collideTime < gameTime->GetTime())
				_rockman->OnCollideWith(_items[i], normalX, normalY, collideTime);
		}

		for (int i = 0; i < _elevators.size(); i++)		// Kiểm tra va chạm với các băng chuyền
		{
			_elevators[i]->Update(gameTime);

			// Kiểm tra va chạm với Rockman
			collideTime = CheckCollision(_rockman, _elevators[i], normalX, normalY, gameTime->GetTime());
			if (collideTime < gameTime->GetTime())
				_rockman->OnCollideWith(_elevators[i], normalX, normalY, collideTime);

			collideTime = CheckCollision(_elevators[i], _rockman, normalX, normalY, gameTime->GetTime());
			if (collideTime < gameTime->GetTime())
				_elevators[i]->OnCollideWith(_rockman, normalX, normalY, gameTime->GetTime() - collideTime);

			for (int j = 0; j < _groundObjs.size(); j++)		// Kiểm tra va chạm với các khối tường, gạch đá
			{
				collideTime = CheckCollision(_elevators[i], _groundObjs[j], normalX, normalY, gameTime->GetTime());
				if (collideTime < gameTime->GetTime())
					_elevators[i]->OnCollideWith(_groundObjs[j], normalX, normalY, collideTime);
			}
			if (!_elevators[i]->GetCollideRegion().IntersecWith(_camera->getViewPort()))
			{
				_elevators.erase(_elevators.begin() + i);
				i -= 1;
			}
		}

		if (_rockman->IsInBossRoom() && _prepareForBoss == 2 && _changingScreen == 0)
		{
			for (int i = 0; i < _enemies.size(); i++)
			{
				switch (_enemies[i]->_typeID)
				{
				case ID_CUTMAN:
					_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_BLOOD_CUTMAN, _enemies[i], ResourceManager::GetSprite(ID_SPRITE_BAR_CUT_VERTICAL), D3DXVECTOR2(43, -37), 0, BLOOD_CUTMAN, 0));
					ResourceManager::StopSound(ID_SOUND_CUTMAN_STAGE);
					ResourceManager::PlayASound(ID_SOUND_BOSS);
					_prepareForBoss = 3;
					return;
				case ID_BOOMMAN:
					_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_BLOOD_BOOMMAN, _enemies[i], ResourceManager::GetSprite(ID_SPRITE_BAR_BOOM_VERTICAL), D3DXVECTOR2(43, -37), 0, BLOOD_BOOMMAN, 0));
					ResourceManager::StopSound(ID_SOUND_BOMBMAN_STAGE);
					ResourceManager::PlayASound(ID_SOUND_BOSS);
					_prepareForBoss = 3;
					return;
				case ID_GUTSMAN:
					_powerEnegies.push_back(new CPowerEnergyX(ID_BAR_BLOOD_GUTSMAN, _enemies[i], ResourceManager::GetSprite(ID_SPRITE_BAR_GUTS_VERTICAL), D3DXVECTOR2(43, -37), 0, BLOOD_GUTSMAN, 0));
					ResourceManager::StopSound(ID_SOUND_GUTSMAN_STAGE);
					ResourceManager::PlayASound(ID_SOUND_BOSS);
					_prepareForBoss = 3;
					return;
				}
			}
		}
		else if (_rockman->IsRequireStopScreen())
		{
			_isStopScreen = true;
			_deltaTimeStopScreen = 0.0f;
			return;
		}
		else if (_rockman->IsEndGame())
		{
			_deltaTimeStopScreen = 0.0f;
			_playState = PlayState::WIN;
			ResourceManager::StopSound(ID_SOUND_BOSS);
			ResourceManager::PlayASound(ID_EFFECT_VICTORY);
			return;
		}

		if (_changingScreen == 0)
		{
			int countRockmanBullet = _bulletsRockman.size();
				for (int i = 0; i < countRockmanBullet; i++)
				{
					for (int j = 0; j < _groundObjs.size(); j++)		// Kiểm tra va chạm với các khối tường, gạch đá
					{
						collideTime = CheckCollision(_bulletsRockman[i], _groundObjs[j], normalX, normalY, gameTime->GetTime());
						if (collideTime < gameTime->GetTime())
							((CBullet*)_bulletsRockman[i])->OnCollideWith(_groundObjs[j], normalX, normalY, collideTime);
					}
					for (int j = 0; j < _enemies.size(); j++)		// Kiểm tra va chạm với các khối tường, gạch đá
					{
						collideTime = CheckCollision(_bulletsRockman[i], _enemies[j], normalX, normalY, gameTime->GetTime());
						if (collideTime < gameTime->GetTime())
							((CBullet*)_bulletsRockman[i])->OnCollideWith(_enemies[j], normalX, normalY, collideTime);
					}
					if (_door != NULL)
					{
						collideTime = CheckCollision(_bulletsRockman[i], _door, normalX, normalY, gameTime->GetTime());
						if (collideTime < gameTime->GetTime())
							((CBullet*)_bulletsRockman[i])->OnCollideWith(_door, normalX, normalY, collideTime);
					}
					_bulletsRockman[i]->Update(gameTime);
					if (_bulletsRockman[i]->GetState() == BULLET_BASE_STATE::DIE || (!_bulletsRockman[i]->GetBox().IntersecWith(Box(_camera->getViewPort())) && _bulletsRockman[i]->GetBox()._y - _camera->getViewPort().top < 0))
					{
						_bulletsRockman[i]->Destroy();
						_bulletsRockman.erase(_bulletsRockman.begin() + i);
						i -= 1;
						countRockmanBullet -= 1;
					}
				}

				for (int i = 0; i < _enemies.size(); i++)			// Kiểm tra va chạm với các đối tượng quái trên màn hình
				{
					collideTime = CheckCollision(_rockman, _enemies[i], normalX, normalY, gameTime->GetTime());
					if (collideTime < gameTime->GetTime())
					{
						//_rockman->OnCollideWith(_enemies[i], normalX, normalY, collideTime);
						//((CEnemy*)_enemies[i])->OnCollideWith(_rockman, normalX, normalY, collideTime);
					}
				}

				for (int i = 0; i < _bulletsEnemy.size(); i++)			// Kiểm tra va chạm với đạn của quái trên màn hình
				{
					if (((CBullet*)_bulletsEnemy[i])->GetState() == BULLET_BASE_STATE::FLYING)
					{
						collideTime = CheckCollision(_rockman, _bulletsEnemy[i], normalX, normalY, gameTime->GetTime());
						if (collideTime < gameTime->GetTime())
						{
							//_rockman->OnCollideWith(_bulletsEnemy[i], normalX, normalY, collideTime);
							//((CBullet*)_bulletsEnemy[i])->OnCollideWith(_rockman, normalX, normalY, collideTime);
						}
					}
				}
				//-----------------------------------------------------------------------------
				// Cập nhật quái và đạn của quái
				//
				//-----------------------------------------------------------------------------
				int countEnemies = _enemies.size();
				for (int i = 0; i < countEnemies; i++)
				{
					_enemies[i]->Update(gameTime);
					_enemies[i]->Update(gameTime, _rockman);
					_enemies[i]->UpdateCamera(*_camera);
					if (_enemies[i]->IsDied() && !_rockman->IsDied())
					{
						if (_enemies[i]->_typeID == ID_CUTMAN)
						{
							CExplodingEffectX* explode = new CExplodingEffectX(_enemies[i]->_position, ResourceManager::GetSprite(ID_SPRITE_EXPLODING_EFFECT_BASE), false);
							explode->SetSoundEffect(ID_EFFECT_BOSS_DIE);
							CExplodingEffectManager::Add(explode);
							_items.push_back(new CItemObject(ID_ITEM_BOSS_CUT,_camera->_listPoint.at(_camera->_listPoint.size()-1) + D3DXVECTOR2(0, SCREEN_HEIGHT / 2), true));
							_items[0]->Initlize();
							_isBossDied = true;
							_clearPoint = _enemies[i]->GetScore();
						}
						else if (_enemies[i]->_typeID == ID_BOOMMAN)
						{
							CExplodingEffectX* explode = new CExplodingEffectX(_enemies[i]->_position, ResourceManager::GetSprite(ID_SPRITE_EXPLODING_EFFECT_BASE), false);
							explode->SetSoundEffect(ID_EFFECT_BOSS_DIE);
							CExplodingEffectManager::Add(explode);
							_items.push_back(new CItemObject(ID_ITEM_BOSS_BOOM, _camera->_listPoint.at(_camera->_listPoint.size()-1) + D3DXVECTOR2(0, SCREEN_HEIGHT / 2), true));
							_items[0]->Initlize();
							_isBossDied = true;
							_clearPoint = _enemies[i]->GetScore();
						}
						else if (_enemies[i]->_typeID == ID_GUTSMAN)
						{
							CExplodingEffectX* explode = new CExplodingEffectX(_enemies[i]->_position, ResourceManager::GetSprite(ID_SPRITE_EXPLODING_EFFECT_BASE), false);
							explode->SetSoundEffect(ID_EFFECT_BOSS_DIE);
							CExplodingEffectManager::Add(explode);
							_items.push_back(new CItemObject(ID_ITEM_BOSS_GUTS, _camera->_listPoint.at(_camera->_listPoint.size()-1) + D3DXVECTOR2(0, SCREEN_HEIGHT / 2), true));
							_items[0]->Initlize();
							_isBossDied = true;
							_clearPoint = _enemies[i]->GetScore();
						}
						else
						{
							CExplodingEffectManager::Add(new CExplodingEffect(_enemies[i]->_position, ResourceManager::GetSprite(ID_SPRITE_EXPLODING_EFFECT_BASE)));

							srand(time(NULL));
							int rand = std::rand() % 7 + 1;
							if (rand == 1)
							{
								CItemObject *item = new CItemObject(ID_ITEM_BLOOD_SMALL, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
							else if (rand == 2)
							{
								CItemObject *item = new CItemObject(ID_ITEM_BLOOD_BIG, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
							else if (rand == 3)
							{
								CItemObject *item = new CItemObject(ID_ITEM_MANA_BIG, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
							else if (rand == 4)
							{
								CItemObject *item = new  CItemObject(ID_ITEM_MANA_SMALL, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
							else if (rand == 5)
							{
								CItemObject *item = new CItemObject(ID_ITEM_BONUS, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
							else if (rand == 7)
							{
								CItemObject *item = new CItemObject(ID_ITEM_LIFE, _enemies[i]->_position);
								item->Initlize();
								_items.push_back(item);
							}
						}
						CGameInfo::GetInstance()->SetScore(CGameInfo::GetInstance()->GetScore() + _enemies[i]->GetScore());
						_enemies.erase(_enemies.begin() + i);
						i -= 1;
						countEnemies -= 1;
					}
					else if ((!_enemies[i]->GetBox().IntersecWith(Box(_camera->getViewPort())) && _enemies[i]->IsInViewPort())
						|| (!_enemies[i]->GetCollideRegion().IntersecWith(Box(_camera->getViewPort())) && !_enemies[i]->IsInViewPort()))
					{
						_enemies.erase(_enemies.begin() + i);
						i -= 1;
						countEnemies -= 1;
					}
					else
					{
						vector<CBullet*> bullets = _enemies[i]->Fire();
						for (int j = 0; j < bullets.size(); j++)
							_bulletsEnemy.push_back(bullets[j]);

						// Kiểm tra quái va chạm với đạn của rockman trên màn hình
						for (int j = 0; j < _bulletsRockman.size(); j++)
						{
							if (_bulletsRockman[j]->GetState() == BULLET_BASE_STATE::FLYING)
							{
								collideTime = CheckCollision(_enemies[i], _bulletsRockman[j], normalX, normalY, gameTime->GetTime());
								if (collideTime < gameTime->GetTime())
								{
									((CEnemy*)_enemies[i])->OnCollideWith(_bulletsRockman[j], normalX, normalY, collideTime);
								}
							}
						}
						// Kiểm tra quái va chạm với tường, gạch đá trên màn  hình

						for (int j = 0; j < _groundObjs.size(); j++)
						{
							collideTime = CheckCollision(_enemies[i], _groundObjs[j], normalX, normalY, gameTime->GetTime());
							if (collideTime < gameTime->GetTime())
								((CEnemy*)_enemies[i])->OnCollideWith(_groundObjs[j], normalX, normalY, collideTime);
						}

						if (_door != NULL)
						{
							collideTime = CheckCollision(_enemies[i], _door, normalX, normalY, gameTime->GetTime());
							if (collideTime < gameTime->GetTime())
								((CEnemy*)_enemies[i])->OnCollideWith(_door, normalX, normalY, collideTime);
						}
					}
				}

				// Cập nhật item
				int countItem = _items.size();
				for (int i = 0; i < countItem; i++)
				{
					if (_items[i]->IsGot() || !(_items[i]->GetCollideRegion().IntersecWith(_camera->getViewPort())))
					{
						_items.erase(_items.begin() + i);
						countItem -= 1;
						i -= 1;
					}
					else
					{
						_items[i]->Update(gameTime);
						for (int j = 0; j < _groundObjs.size(); j++)
						{
							collideTime = CheckCollision(_items[i], _groundObjs[j], normalX, normalY, gameTime->GetTime());
							if (collideTime < gameTime->GetTime())
								_items[i]->UpdateCollision(_groundObjs[j], normalX, normalY, collideTime, gameTime->GetTime() - collideTime);
						}
					}
				}

				// Cập nhật đạn của quái, xóa các đạn chuyển sang trạng thái DIE
				int countBulletEnemy = _bulletsEnemy.size();
				for (int i = 0; i < countBulletEnemy; i++)
				{
					if (_bulletsEnemy[i]->_typeID != ID_BULLET_CUTMAN)
					{
						_bulletsEnemy[i]->Update(gameTime);

						// Kiểm tra quái va chạm với tường, gạch đá trên màn  hình
						for (int j = 0; j < _groundObjs.size(); j++)
						{
							collideTime = CheckCollision(_bulletsEnemy[i], _groundObjs[j], normalX, normalY, gameTime->GetTime());
							if (collideTime < gameTime->GetTime())
								((CBullet*)_bulletsEnemy[i])->OnCollideWith(_groundObjs[j], normalX, normalY, collideTime);
						}
					}

					if (_bulletsEnemy[i]->GetState() == BULLET_BASE_STATE::DIE || !_bulletsEnemy[i]->GetBox().IntersecWith(Box(_camera->getViewPort())))
					{
						_bulletsEnemy[i]->Destroy();
						_bulletsEnemy.erase(_bulletsEnemy.begin() + i);
						countBulletEnemy -= 1;
						i -= 1;
					}
				}

			}




			for (int i = 0; i < _powerEnegies.size(); i++)
			{
				_powerEnegies[i]->UpdateCamera(_camera);

				switch (_powerEnegies[i]->_typeID)
				{
				case ID_BAR_BLOOD_ROCKMAN:
					_powerEnegies[i]->SetValue(_rockman->_blood);
					break;
				case ID_BAR_WEAPONS_CUT:
				case ID_BAR_WEAPONS_GUTS:
				case ID_BAR_WEAPONS_BOOM:
					_powerEnegies[i]->SetValue(_rockman->GetWeapons(_rockman->GetCurrentSkill()));
					break;
				case ID_BAR_BLOOD_CUTMAN:
				case ID_BAR_BLOOD_GUTSMAN:
				case ID_BAR_BLOOD_BOOMMAN:
					if (_prepareForBoss == 3)
						_powerEnegies[i]->SetValue(__max(0, _powerEnegies[i]->GetMaster()->_blood));
					break;
				}
				_powerEnegies[i]->Update(gameTime);

				if (!_powerEnegies[i]->IsCompleted())
				{
					_playState = PlayState::PAUSE;
				}
			}
		#pragma endregion
		}
		else if(_playState == PlayState::GAMEOVER)
		{
#pragma region Xử lý game over, có thể là Rockman mất mạng hoặc GameOver
			CInput::GetInstance()->Active();
			//int startIndex = _camera->_listPoint.at(0);
			//LoadMap();

			if (_rockman->GetLife() == -1)
			{
				_isFinished = true;
				_nextScreen = new CGameOver(CGameInfo::GetInstance()->GetScore(), this->_camera);
			}
			else
			{
				//FindScene(startIndex);

				// Khởi động trạng thái
				_playState = PlayState::READY;
				_deltaTime = 0;
				_rockman->_behave = Behave::START;
				_prepareForBoss = 0;
				_isBossDied = false;

				_enemies.clear();
				_bulletsRockman.clear();
				_bulletsEnemy.clear();
				_powerEnegies.clear();
				_items.clear();

				_pointAfterDoor = D3DXVECTOR2(0, 0);
				_pointBeforeDoor = D3DXVECTOR2(0, 0);
				_shakePointRand = D3DXVECTOR2(0.0f, 0.0f);
				CInput::GetInstance()->Active();

				switch (CGameInfo::GetInstance()->GetLevel())
				{
				case ID_LEVEL_BOOM:
					ResourceManager::PlayASound(ID_SOUND_BOMBMAN_STAGE);
					break;
				case ID_LEVEL_CUT:
					ResourceManager::PlayASound(ID_SOUND_CUTMAN_STAGE);
					break;
				case ID_LEVEL_GUTS:
					ResourceManager::PlayASound(ID_SOUND_GUTSMAN_STAGE);
					break;
				}
			}
#pragma endregion
		}
		else if (_playState == PlayState::PAUSE)
		{
#pragma region Xử lý dừng game
			bool isCompleted = true;
			for (int i = 0; i < _powerEnegies.size(); i++)
			{
				_powerEnegies[i]->Update(gameTime);
				if (!_powerEnegies[i]->IsCompleted())
					isCompleted = false;
			}
			if (isCompleted)
			{
				CInput::GetInstance()->Active();
				_playState = PlayState::PLAYING;
			}
#pragma endregion

		}

		else if (_playState == PlayState::WIN)
		{
#pragma region Xử lý chiến thắng game

			_deltaTime += gameTime->GetTime();
			if (_deltaTime >= 6000)
			{

				// Vẽ clear point
				if (_deltaTime >= 6000 && _deltaTime < 10000)
				{
					_strClearPoint = CGameInfo::GetInstance()->to_string(1000, 6);
					if (_deltaTime > 7000)
						_deltaTime = 10000;
				}

				// Vẽ điểm tăng dần
				if (_deltaTime >= 10000 && _deltaTime < 20000)
				{
					_deltaClearPoint += 200;
					ResourceManager::PlayASound(ID_EFFECT_ANIMATION_SCORE);
					CGameInfo::GetInstance()->SetScore(CGameInfo::GetInstance()->GetScore() + 200);
					if (_deltaClearPoint > _clearPoint)
					{
						_deltaClearPoint = _clearPoint;
						CGameInfo::GetInstance()->SetScore(CGameInfo::GetInstance()->GetScore() - 200);
						_totalScore = CGameInfo::GetInstance()->GetScore();
						_deltaTime = 20000;
					}
					_strClearPoint = CGameInfo::GetInstance()->to_string(_deltaClearPoint, 6);
				}

				// Vẽ bonus, dừng khoảng 1s
				if (_deltaTime >= 20000 && _deltaTime < 30000)
				{
					ResourceManager::StopSound(ID_EFFECT_ANIMATION_SCORE);
					_strBonus = std::to_string(CGameInfo::GetInstance()->GetBonusValue()) + "X00";
					_strTotalBonusScore = CGameInfo::GetInstance()->to_string(0, 6);
					if (_deltaTime > 20100)
						_deltaTime = 30000;
				}

				// Vẽ bonus tăng dần
				if (_deltaTime >= 30000 && _deltaTime < 50000)
				{
					_strBonus = std::to_string(CGameInfo::GetInstance()->GetBonusValue()) + "X" + std::to_string(CGameInfo::GetInstance()->GetTotalBonus());
					_strTotalBonusScore = CGameInfo::GetInstance()->to_string(CGameInfo::GetInstance()->GetBonusValue()*CGameInfo::GetInstance()->GetTotalBonus(), 6);

					CGameInfo::GetInstance()->SetScore(_totalScore + CGameInfo::GetInstance()->GetTotalBonus()* CGameInfo::GetInstance()->GetBonusValue());

					if (_deltaTime > 35000)
					{
						ResourceManager::StopSound(ID_SOUND_BOSS);
						switch (CGameInfo::GetInstance()->GetLevel())
						{
						case ID_LEVEL_CUT:
							CGameInfo::GetInstance()->AddSkill(Skill::CUT);
							break;
						case ID_LEVEL_GUTS:
							CGameInfo::GetInstance()->AddSkill(Skill::GUTS);
							break;
						case ID_LEVEL_BOOM:
							CGameInfo::GetInstance()->AddSkill(Skill::BOOM);
							break;

						}
						CGameInfo::GetInstance()->SetBonus(0);
						CGameInfo::GetInstance()->Save();
						_isFinished = true;
						_nextScreen = new CSelectBossState( MGraphic::GetInstance(),  MGraphic::GetInstance()->GetDevice());
					}
				}

#pragma endregion
			}
		}

			CExplodingEffectManager::Update(gameTime);
	//// update cac doi tuong
	//for (int i = 0; i < _groundObjs.size(); ++i)
	//{
	//	_groundObjs.at(i)->Update(gameTime);
	//	_groundObjs.at(i)->UpdateBox();
	//}

	//for (int i = 0; i < _elevators.size(); ++i)
	//{
	//	_elevators.at(i)->Update(gameTime);
	//	_elevators.at(i)->UpdateBox();
	//}

	//for (int i = 0; i <  _enemies.size(); ++i)
	//{
	//	if (_enemies.at(i)->GetBox().IntersecWith(_camera->getViewPort()))
	//	{
	//		_enemies.at(i)->Update(gameTime, _rockman);
	//		_enemies.at(i)->UpdateBox();
	//	}
	//	
	//}

	//for (int i = 0; i < _items.size(); i++)
	//{
	//	_items.at(i)->Update(gameTime);
	//	_items.at(i)->UpdateBox();
	//}
}

	
	
	
}

void CPlayState::Draw(CTimer* gameTime, MGraphic* graphics)
{
	//tileManager->RenderTile(graphics,this->_camera);
	

	_map->Render(graphics,this->_camera);
	
	tileManager->RenderTile(graphics,this->_camera);

	//render object
	/*for(map<int,CGameObject*>::iterator i = tree->_listObjectOnScreen.begin();i != tree->_listObjectOnScreen.end();++i)
	{
		(*i).second->Render(gameTime,graphics);
	}*/

	D3DXVECTOR2 oldCameraPoint = _camera->_pos;

	if (Asset::GetInstance()->__is_require_shake_screen)
	{
		_deltatTimeShakingScreen += gameTime->GetTime();
		if (_deltatTimeShakingScreen > 50)
		{
			_deltatTimeShakingScreen = 0;
			_shakePointRand.x = rand() % 7 + 1;
			_shakePointRand.y = rand() % 3 + 1;

			_camera->_pos = (oldCameraPoint + _shakePointRand);
		}
	}
	RenderBackground(graphics, _camera->getViewPort());

	if (_prepareForBoss == 1)
	{
		graphics->Draw(_spriteIntroBoss.GetTexture(), _spriteIntroBoss.GetDestinationRectangle(), _camera->_listPoint.at(_camera->_listPoint.size()-1), true, D3DXVECTOR2(1.0f, 1.0f), SpriteEffects::NONE);
		_spriteIntroBoss.Update(gameTime);
	}

	for (int i = 0; i < _groundObjs.size(); i++)
		_groundObjs[i]->Render(gameTime, graphics);

	for (int i = 0; i < _listDoor.size(); i++)
	{
		_listDoor.at(i)->Render(gameTime, graphics);
	}
	_camera->_pos = (oldCameraPoint);

	for (int i = 0; i < _bulletsRockman.size(); i++)
		_bulletsRockman[i]->Render(gameTime, graphics);
	for (int i = 0; i < _bulletsEnemy.size(); i++)
		_bulletsEnemy[i]->Render(gameTime, graphics);
	for (int i = 0; i < _enemies.size(); i++)
		_enemies[i]->Render(gameTime, graphics);
	for (int i = 0; i < _items.size(); i++)
		_items[i]->Render(gameTime, graphics);
	for (int i = 0; i < _elevators.size(); i++)
		_elevators[i]->Render(gameTime, graphics);
	if (_playState != PlayState::WIN)
		for (int i = 0; i < _powerEnegies.size(); i++)
			_powerEnegies[i]->Render(gameTime, graphics);
	_rockman->Render(gameTime, graphics);

	// Vẽ điểm màn chơi
	if (_playState == PlayState::READY)
		graphics->DrawString("READY", D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2, _camera->_pos.y - SCREEN_HEIGHT / 2), _defaultStringColor, true,this->_camera);
	else
		graphics->DrawString(CGameInfo::GetInstance()->to_string(CGameInfo::GetInstance()->GetScore(), 7), D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2, _camera->_pos.y - FONT_SIZE), _defaultStringColor, true, this->_camera);

	// Vẽ điểm khi WIN GAME
	if (_playState == PlayState::WIN)
	{
		graphics->DrawString("CLEAR", D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 - FONT_SIZE * 2, _camera->_pos.y - FONT_SIZE * 6), _defaultStringColor, true,this->_camera);
		graphics->DrawString("POINTS", D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 + FONT_SIZE, _camera->_pos.y - FONT_SIZE * 7), _defaultStringColor, true,this->_camera);
		graphics->DrawString(_strClearPoint, D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2, _camera->_pos.y - FONT_SIZE * 9), _defaultStringColor, true,this->_camera);
		if (_strBonus != "")
		{
			graphics->DrawString(_strBonus, D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 + FONT_SIZE, _camera->_pos.y - FONT_SIZE * 11), _defaultStringColor, true, this->_camera);
			graphics->DrawString("BONUS", D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 + FONT_SIZE, _camera->_pos.y - FONT_SIZE * 12), _defaultStringColor, true, this->_camera);
			graphics->DrawString(_strTotalBonusScore, D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 + FONT_SIZE, _camera->_pos.y - FONT_SIZE * 14), _defaultStringColor, true,this->_camera);
			graphics->Draw(ResourceManager::GetSprite(ID_SPRITE_ITEM_BONUS_RED).GetTexture(), ResourceManager::GetSprite(ID_SPRITE_ITEM_BONUS_RED).GetDestinationRectangle(), D3DXVECTOR2(_camera->_pos.x + SCREEN_WIDTH / 2 - 4 * FONT_SIZE, _camera->_pos.y - FONT_SIZE * 12), true);
		}
	}

	if (!_isStopScreen)
		CExplodingEffectManager::Render(gameTime, graphics);



	//tree->Render(gameTime,graphics);
	//listBubble.at(0)->Render(gameTime,graphics);
	//bubble->Render(gameTime,graphics);
	//_enemies.at(4)->Render(gameTime, graphics);

	_rockman->Render(gameTime, graphics);


}

CPlayState::~CPlayState(void)
{
}

void CPlayState::RenderBackground(MGraphic* graphics, RECT viewport)
{

}

float CPlayState::CheckCollision(CGameObject* obj1, CGameObject* obj2, CDirection &normalX, CDirection &normalY, float frameTime)
{
	float timeCollision = frameTime;
	Box moveBox = obj1->GetBox();
	Box staticBox = obj2->GetBox();

	// Cố định lại box của object và tính lại vận tốc của box nội tại
	moveBox._vx -= staticBox._vx;
	moveBox._vy -= staticBox._vy;
	staticBox._vx = 0;
	staticBox._vy = 0;


	// tạo broad phase box để kiểm tra tổng quát với đối tượng đứng yên obj
	// Nếu có xảy ra va chạm, thì tiến hành kiểm tra bằng AABBSweep và đưa ra thời gian va chạm
	Box broadBox = CCollision::GetSweptBox(moveBox, frameTime);
	if (CCollision::AABBCheck(staticBox, broadBox))
		timeCollision = CCollision::SweepAABB(moveBox, staticBox, normalX, normalY, frameTime);

	return timeCollision;
}

void CPlayState::ChangeScreen(CDirection direction)
{

}

void CPlayState::FindScene(unsigned int startIndex)
{

}




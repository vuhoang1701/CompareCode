#ifndef _CPLAY_STATE_H_
#define _CPLAY_STATE_H_

#pragma once
#include <map>
#include "Screen.h"
#include "Tile.h"
#include "Map.h"
#include "Megaman.h"
#include "QuadTree.h"
#include "CGameInfo.h"
#include "CGameOver.h"
using namespace std;

enum PlayState
{
	READY,
	PLAYING,
	PAUSE,
	GAMEOVER,
	WIN
};

class CPlayState: public CScreen

{
public:
	CPlayState(char *pathmap, MGraphic* gra, LPDIRECT3DDEVICE9 d3ddev, MKeyboard* input,  Camera* cam, int id);

	//-----------------------------------------------------------------------------
	// Phương thức cập nhật sự kiện bàn phím, chuột
	//-----------------------------------------------------------------------------
	void UpdateKeyboard(MKeyboard* input) override;

	//-----------------------------------------------------------------------------
	// Phương thức cập nhật màn hình theo thời gian thực
	//-----------------------------------------------------------------------------
	void Update(CTimer* gameTime) override;

	//-----------------------------------------------------------------------------
	// Phương thức vẽ màn hình và các thành phần con bên trong
	//-----------------------------------------------------------------------------
	void Draw(CTimer* gameTime, MGraphic* graphics) override;

	~CPlayState(void);

private:
	char* mappath;

	//khai báo megaman


	//khai báo map
	Map* _map;

	//Tile
	CTile* tileManager;

		//Khai báo đối tượng Content(Dùng để load dữ liệu)
	MContent* content;

	

	//-----------------------------------------------------------------------------
	// Phương thức vẽ bacground lên màn hình dựa theo khung camera di chuyển
	//-----------------------------------------------------------------------------
	void RenderBackground(MGraphic* graphics, RECT viewport);

	void LoadMap();

	//-----------------------------------------------------------------------------
	// Cập nhật va chạm của đối tượng với các đối tượng khác trong game
	//
	// + obj1: đối tượng cần để kiểm tra va chạm
	// + obj2: đối tượng cần để kiểm tra va chạm
	// + normalX: Hướng va chạm theo chiều x
	// + normalY: Hướng va chạm theo chiều y
	// + frameTime: Khoảng thời gian chuyển khung hình
	//
	//-----------------------------------------------------------------------------
	float CheckCollision(CGameObject* obj1, CGameObject* obj2, CDirection &normalX, CDirection &normalY, float frameTime);

	void ChangeScreen(CDirection direction);

	void FindScene(unsigned int startIndex);

	vector<vector<int>>		_tileMatrix;	// Ma trận lưu các tile background
	int						_countRow;		// Số lượng dòng của ma trận
	int						_countColumn;	// Số lượng cột của ma trận
	int						_totalTile;		// Tổng số tile được cắt
	CTexture				_textureBkg;	// Đối tượng nắm giữ ảnh nền
	
	Megaman*				_rockman;		// Đối tượng Rockman
	vector<CBullet*>		_bulletsEnemy;	// Danh sách các đối tượng đạn của quái
	vector<CBullet*>		_bulletsRockman;// Danh sách các đối tượng đạn của Rockman
	vector<CGameObject*>	_groundObjs;	// Danh sách các đối tượng nằm không thể phá hủy bởi bất kì hình thức gì, bao gồm cầu thang, nền, tường, cửa qua màn, gai chết người
	vector<CElevator*>		_elevators;		// Danh sách băng chuyền trong game
	vector<CEnemy*>			_enemies;		// Danh sách các quái nằm trong vùng va chạm
	vector<CItemObject*>			_items;			// Danh sách các item nằm trong vùng va chạm
	vector<CPowerEnergyX*>	_powerEnegies;	// Danh sách ống năng lượng

	//CQuadNode	_quadNodeCollision;			// Cây tứ phân lưu các đối tượng va chạm trên màn hình
	int			_changingScreen;			// Kiếm tra có đang chuyển frame màn hình hay không
	int			_prepareForBoss;			// Kiểm tra có đang chuẩn bị giết boss hay không
	int			_doorState;					// Kiểm tra có đang mở cửa giết trùm hay chưa, -1 là đang mở cửa, 0 là đã mở cửa, 1 là đang đóng cửa

	D3DXVECTOR2		_screenPosition, _newScreenPosition, _pointDoor, _pointAfterDoor, _pointBeforeDoor;
	CDirection	_changeScreenDirection;

	PlayState	_playState;					// Trạng thái màn chơi
	int			_deltaTime;					// Biến đếm thời gian theo tick
	bool		_isStopScreen;
	bool		_isChosingSkill;			// Xác nhận cửa sổ popup có hiện hay không
	bool		_isPause;					// Dừng toàn màn hình
	bool		_isBossDied;				// Kiểm tra boss có chết hay chưa
	int			_deltaTimeStopScreen;
	CSprite		_spriteIntroBoss;

	int			_deltatTimeShakingScreen;
	D3DXVECTOR2		_shakePointRand;
	CDoor*		_door;
	vector<CDoor*> _listDoor;
	// Các biến điều khiển việc tính điểm màn hình
	int			_deltaClearPoint, _clearPoint, _totalScore;			// Biến tăng giá trị để tính điểm trên màn hình
	string		_strClearPoint;
	string		_strBonus, _strTotalBonusScore;
	D3DCOLOR		_defaultStringColor;
	CQuadTree* tree;
	// enemy for test

	CEnemyBubble* bubble;

	vector<CEnemyBubble*> listBubble;
};

#endif


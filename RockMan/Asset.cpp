#include "Asset.h"

Asset::Asset()
{
}

Asset::~Asset()
{
	__is_require_shake_screen = false;
}

Asset* Asset::_pInstance = new Asset();

Asset* Asset::GetInstance()
{
	if (_pInstance == nullptr)
		_pInstance = new Asset();
	return _pInstance;
};
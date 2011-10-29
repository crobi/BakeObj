#include "packer.h"

#include <set>
#include <list>
#include <map>
#include <algorithm>

#include "IL/il.h"
#include "IL/ilu.h"
#include "IL/ilut.h"

class TextureTile
{
public:
	const TextureTile* parent;
	int                sizeX;
	int                sizeY;
public:
	inline int maxSize() const { return std::max<int>(sizeX,sizeY); } 
	inline int area() const { return sizeX*sizeY; }
public:
	virtual bool getTileOffset(const TextureTile* tile, int offsetX, int offsetY, int& resultX, int& resultY) const = 0;

	TextureTile()
	{
		parent = NULL;
		sizeX = 0;
		sizeY = 0;
	}
	virtual ~TextureTile(){};
};
typedef std::list<TextureTile*> TextureTileListType;
typedef std::vector<TextureTile*> TextureTileArrayType;

bool operator<(const TextureTile& a, const TextureTile& b)
{
	return a.maxSize() < b.maxSize();
}

bool compareTiles(const TextureTile* a, const TextureTile* b)
{
	return *a < *b;
}

class TextureTileQuad: public TextureTile
{
public:	
	TextureTileQuad* children[4];
	int              offsetsX[4];
	int              offsetsY[4];
public:
	void addChildren(const TextureTileArrayType& children, TextureTileListType& headTiles);
	virtual bool getTileOffset(const TextureTile* tile, int offsetX, int offsetY, int& resultX, int& resultY) const
	{
		bool result = false;
		for(int i=0; i<4; i++)
		{
			result = result || (children[i]!=NULL && 
				children[i]->getTileOffset(tile, offsetX+offsetsX[i], offsetY+offsetsY[i], resultX, resultY));
		}
	}
	TextureTileQuad()
	{
		for(int i=0; i<4;i++)
		{
			children[i] = NULL;
		}
	}
};

class TextureTileLeaf: public TextureTile
{
public:
	int exactWidth;
	int exactHeight;
	ILuint image;

public:
	void loadFromFile(const std::string& filename);
	virtual bool getTileOffset(const TextureTile* tile, int offsetX, int offsetY, int& resultX, int& resultY) const
	{
		if (tile==this)
		{
			resultX = offsetX;
			resultY = offsetY;
			return true;
		}
		else return false;
	}
	TextureTileLeaf()
	{
		exactWidth = 0;
		exactHeight = 0;
		image = 0;
	}
};

void packTextures(const Mesh& inputMesh, Mesh& outputMesh)
{
	// Collect all actually used materials
	std::set<std::string> usedMaterialNames;
	for(ComponentListType::const_iterator ic=inputMesh.components.begin();ic!=inputMesh.components.end();++ic)
	{
		usedMaterialNames.insert(ic->materialName);
	}

	// Create a list of all tiles
	TextureTileListType headTiles;
	TextureTileListType allTiles;

	// Create one leaf for each input material texture
	std::map<std::string, TextureTileLeaf*> materialTiles;
	for(MaterialMapType::const_iterator im=inputMesh.materials.begin();im!=inputMesh.materials.end();++im)
	{
		const std::string name = im->first;
		const Material& mat = im->second;
		if (!mat.textureDiffuse.empty() && usedMaterialNames.find(name)!=usedMaterialNames.end())
		{
			TextureTileLeaf* leaf = new TextureTileLeaf();
			headTiles.push_back(leaf);
			allTiles.push_back(leaf);
			leaf->loadFromFile(mat.textureDiffuse);
			materialTiles[name] = leaf;
		}
	}

	// Combine tiles until only one image remains
	std::vector<TextureTile*> candidateTiles;
	candidateTiles.reserve(8);
	while(headTiles.size()>0)
	{
		headTiles.sort(compareTiles);

		// Find candidates to combine
		TextureTile* smallestTile = headTiles.front();
		candidateTiles.clear();
		for(TextureTileListType::iterator it=headTiles.begin(); it!=headTiles.end(); ++it)
		{
			if (smallestTile->maxSize() == it->maxSize())
			{
				candidateTiles.push_back(*it);
			}
		}

		// Create a new tile
		TextureTileQuad* quad = new TextureTileQuad();
		headTiles.push_back(quad);
		allTiles.push_back(quad);
		quad->addChildren(candidateTiles, headTiles);
	}

	// Transform texture coordinates

	// Stitch the texture atlas
}

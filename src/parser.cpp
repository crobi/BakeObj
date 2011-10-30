#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

const bool normalizeNormals = true;

//=================================================================================================
// Check if a character is a decimal
//=================================================================================================
inline bool isDecimal(char c)
{
	return c != 0 && c >= '0' && c <= '9';
}

//=================================================================================================
// Check if a character is a whitespace
//=================================================================================================
inline bool isWhitespace(char c)
{
	return c == ' ' || c=='\t';
}

//=================================================================================================
// Check if a character is a forward slash
//=================================================================================================
inline bool isSlash(char c)
{
	return c == '/';
}

//=================================================================================================
// Converts a character to a integer
//=================================================================================================
inline int charToInt(char c)
{
	return static_cast<int>(c) - static_cast<int>('0');
}

//=================================================================================================
// Consumes a slash
//=================================================================================================
inline bool parseSlash(const char*& str)
{
	if(isSlash(*str))
	{
		str++;
		return true;
	}
	else
	{
		return false;
	}
}

//=================================================================================================
// Consumes all whitespaces
//=================================================================================================
inline bool parseWhitespace(const char*& str)
{
	if(!isWhitespace(*str))
	{
		return false;
	}
	while(isWhitespace(*str))
	{
		str++;
	}
	return true;
}

//=================================================================================================
// Consumes a number
//=================================================================================================
bool parseNumber(const char*& str, int& result)
{
	if (!isDecimal(*str))
	{
		return false;
	}

	result = 0;
	while(isDecimal(*str))
	{
		result = result*10 + charToInt(*str);
		str++;
	}

	return true;
}

//=================================================================================================
// Consumes a vertex definition inside a face definition
//=================================================================================================
bool parseVertex(const char*& str, int& vertex, int& normal, int& texcoord)
{
	if(!parseNumber(str, vertex))
	{
		return false;
	}

	normal = vertex;
	texcoord = vertex;

	if(parseSlash(str))
	{
		parseNumber(str, texcoord);
	}

	if(parseSlash(str))
	{
		parseNumber(str, normal);
	}

	return true;
}

//=================================================================================================
// Consumes a face definition
//=================================================================================================
bool parseFace(const char*& str, Vector3i* indices, int& vertexCount, int maxVertexCount)
{
	parseWhitespace(str);

	vertexCount = 0;

	int indexV = 0, indexN = 0, indexT = 0;

	while(parseVertex(str, indexV, indexN, indexT) && vertexCount < maxVertexCount)
	{
		indices[vertexCount].data[0] = indexV;
		indices[vertexCount].data[1] = indexN;
		indices[vertexCount].data[2] = indexT;

		vertexCount++;

		parseWhitespace(str);
	}

	return true;
}

//=================================================================================================
// Parial ordering of faces
//=================================================================================================
struct CompareFaces 
{
	bool operator() (const Vector3i &a, const Vector3i &b) const
	{ 
		if (a.data[0] > b.data[0])
		{
			return true;
		}
		if (a.data[0] < b.data[0])
		{
			return false;
		}
		if (a.data[1] > b.data[1])
		{
			return true;
		}
		if (a.data[1] < b.data[1])
		{
			return false;
		}

		return a.data[2] > b.data[2];
	}
};

//=================================================================================================
// Parses a color
//=================================================================================================
bool parseColor(std::istringstream& stream, Vector3f& color)
{
	for(int i=0; i<3; i++)
	{
		stream >> color.data[i];
	}
	return true;
}

//=================================================================================================
// Loads a material
// newmtl materialName
// illum 2
// Kd 0.000000 0.000000 0.000000
// Ka 0.250000 0.250000 0.250000
// Ks 1.000000 1.000000 1.000000
// Ke 0.000000 0.000000 0.000000
// Ns 0.000000
// map_Kd textureFileName.tga
//=================================================================================================
void loadMaterialFile(const std::string& filename, MaterialMapType& materials)
{
	// Open the file
	std::ifstream infile;
	infile.open(filename.c_str());
	if(!infile.is_open())
	{
		throw std::runtime_error("Unable to open material file: " + filename);
	}

	std::string line;
	int linecount = 0;
	Material currentMaterial;
	std::string currentName;

	while(getline(infile, line))
	{
		linecount++;

		// Skip comments and empty lines
		if(line.length()==0 || line[0] == '#')
		{
			continue;
		}

		// Streamed reads are easier
		std::istringstream stream(line);
		std::string type;

		// Read the command
		stream >> type;
		std::transform(type.begin(), type.end(), type.begin(), tolower); 

		if(type == "newmtl")
		{
			if (!currentName.empty())
			{
				if (materials.find(currentName)!=materials.end())
				{
					std::cerr << "duplicate material definition for " << currentName << std::endl;
				}
				materials[currentName] = currentMaterial;
			}
			currentMaterial.reset();
			stream >> currentName;
		}
		else if (type == "illum")
		{
			stream >> currentMaterial.illuminationModel;
		}
		else if (type == "kd")
		{
			parseColor(stream, currentMaterial.colorDiffuse);
		}
		else if (type == "ks")
		{
			parseColor(stream, currentMaterial.colorSpecular);
		}
		else if (type == "ka")
		{
			parseColor(stream, currentMaterial.colorAmbient);
		}
		else if (type == "ke")
		{
			parseColor(stream, currentMaterial.colorEmissive);
		}
		else if (type == "ns")
		{
			stream >> currentMaterial.shininess;
		}
		else if (type == "d" || type == "Tr")
		{
			stream >> currentMaterial.transparency;
		}
		else if (type == "map_kd")
		{
			stream >> currentMaterial.textureDiffuse;
		}
		else if (type == "map_ks")
		{
			stream >> currentMaterial.textureSpecular;
		}
		else if (type == "map_ka")
		{
			stream >> currentMaterial.textureAmbient;
		}
		else if (type == "map_ke")
		{
			stream >> currentMaterial.textureEmissive;
		}
		else if (type == "map_bump" || type == "bump")
		{
			stream >> currentMaterial.textureBump;
		}
		else if (type == "map_d" || type == "map_tr")
		{
			stream >> currentMaterial.textureTransparency;
		}
		else
		{
			std::cerr << "unknown material command: " << type << std::endl;
		}
	}

	if (!currentName.empty())
	{
		if (materials.find(currentName)!=materials.end())
		{
			std::cerr << "duplicate material definition for " << currentName << std::endl;
		}
		materials[currentName] = currentMaterial;
	}
}

//=================================================================================================
// Loads an obj file
//=================================================================================================
void loadObj(const std::string& filename, Mesh& result)
{	
	// Clear the result
	result.reset();

	// Open the file
	std::ifstream infile;
	infile.open(filename.c_str());
	if(!infile.is_open())
	{
		throw std::runtime_error("Unable to open mesh file: " + filename);
	}

	std::string line;
	bool hasNormals = false;
	bool hasTextureCoordinates = false;

	// Original mesh data.
	// This might get duplicated if two faces partially share vertex data
	std::vector<Vector3f> vertices;
	std::vector<Vector3f> normals;
	std::vector<Vector2f> texcoord;
	std::map<Vector3i, int, CompareFaces> uniqueVertexMap;

	// Loop over all lines
	int unsupportedTypeWarningsLeft = 10;
	int linecount = 0;
	while(getline(infile, line))
	{
		linecount++;

		// Skip comments and empty lines
		if(line.length()==0 || line[0] == '#')
		{
			continue;
		}

		// Read the command
		std::istringstream stream(line);
		std::string type;

		stream >> type;
		std::transform(type.begin(), type.end(), type.begin(), tolower); 

		if(type == "mtllib")
		{
			std::string materialFileName;
			stream >> materialFileName;
			// FIXME: handle relative and absolute file names
			loadMaterialFile(materialFileName, result.materials);
		}
		else if(type == "g")
		{
			result.components.push_back(MeshComponent());
			stream >> result.components.back().componentName;
		}
		else if(type == "usemtl")
		{
			if(result.components.size()==0)
			{
				throw std::runtime_error("material without a group encountered");
			}
			if (!result.components.back().materialName.empty())
			{
				std::cerr << "component " << result.components.back().componentName << " already has a material, replacing the old definition";
			}
			stream >> result.components.back().materialName;
		}
		else if(type == "s")
		{
			// FIXME: Smooth shading
		}
		else if(type == "o")
		{
			// FIXME: object names (no real use)
		}
		else if(type == "v")
		{
			// Vertex position data
			Vector3f v;
			stream >> v.data[0] >> v.data[1] >> v.data[2];
			vertices.push_back(v);
		} 
		else if(type == "vn")
		{
			// Vertex normal data
			Vector3f vn;
			stream >> vn.data[0] >> vn.data[1] >> vn.data[2];
			if (normalizeNormals)
			{
				float normalLength = vn.data[0]*vn.data[0] + vn.data[1]*vn.data[1] + vn.data[2]*vn.data[2];
				if (abs(1.0f-normalLength) > 1e-3f && normalLength > 1e-3f)
				{
					vn.data[0] /= normalLength;
					vn.data[1] /= normalLength;
					vn.data[2] /= normalLength;
				}
			}

			hasNormals = true;
			normals.push_back(vn);
		} 
		else if(type == "vt")
		{
			// Vertex texture coordinate data
			Vector2f vt;
			stream >> vt.data[0] >> vt.data[1];
			hasTextureCoordinates = true;
			texcoord.push_back(vt);
		} 
		else if(type == "f")
		{
			// Face data
			int pos = (int) stream.tellg();

			const char* ptr = line.c_str() + pos;

			// Every vertex may supply up to 3 indexes (vertex index, normal index, texcoord index)
			Vector3i loadedIndices[3];
			int mappedIndices[3];
			int vertexCount;

			if(parseFace(ptr, loadedIndices, vertexCount, 3))
			{
				// If the face is legal, for every vertex, assign the normal and the texcoord
				for (int i = 0; i < vertexCount; ++i)
				{
					if (uniqueVertexMap.find(loadedIndices[i]) != uniqueVertexMap.end())
					{
						mappedIndices[i] = uniqueVertexMap[loadedIndices[i]];
					}
					else
					{
						mappedIndices[i] = (int) result.vertices.size();
						uniqueVertexMap.insert(std::make_pair(loadedIndices[i], (int) uniqueVertexMap.size()));

						int vertexIndex = loadedIndices[i].data[0] - 1;
						if (vertexIndex < (int) vertices.size())
						{
							result.vertices.push_back(vertices[vertexIndex]);	
						}
						else
						{
							throw std::runtime_error("Unknown vertex specified");
						}

						if (hasNormals)
						{
							int normalIndex = loadedIndices[i].data[1] - 1;

							if (normalIndex < (int) normals.size())
							{
								result.normals.push_back(normals[normalIndex]);	
							}
							else
							{
								throw std::runtime_error("Unknown normal specified");
							}
						}


						if (hasTextureCoordinates)
						{
							int texCoordIndex = loadedIndices[i].data[2] - 1;
							
							if (texCoordIndex < (int) texcoord.size())
							{
								result.texcoord.push_back(texcoord[texCoordIndex]);	
							}
							else
							{
								throw std::runtime_error("Unknown texture coordinate specified");
							}
						}
					}
				}

				if (vertexCount == 3)
				{
					if (result.components.size()==0)
					{
						result.components.push_back(MeshComponent());
						result.components.back().componentName = "[default]";
					}

					Vector3i tri(mappedIndices);
					result.components.back().faces.push_back(tri);
				}
				else if (vertexCount == 4)
				{
					throw std::runtime_error("OBJ loader: quads are not supported, convert them to triangles");
				}
				else
				{
					throw std::runtime_error("OBJ loader: face with strange number of vertices encountered");
				}
			}
			else
			{
				throw std::runtime_error("OBJ loader: face could not be parsed");
			}
		}
		else if (unsupportedTypeWarningsLeft > 0)
		{
			std::cerr << "Unsupported type in obj: " << type << " at line " << linecount << std::endl;
			unsupportedTypeWarningsLeft--;
			if(unsupportedTypeWarningsLeft==0)
			{
				std::cerr << "Too many warnings about unsupported types, further warnings are suppressed." << std::endl;
			}
		}
	}

	if (!hasNormals)
	{
		throw std::runtime_error("OBJ loader: mesh without normals");
	}

	if (result.texcoord.size()==0)
	{
		Vector2f defaultTexCoord;
		defaultTexCoord.data[0] = 0;
		defaultTexCoord.data[1] = 0;
		result.texcoord.resize(result.vertices.size(), defaultTexCoord);
	}

	if (result.normals.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of normals");
	}
	if (result.texcoord.size() != result.vertices.size())
	{
		throw std::runtime_error("OBJ loader: inconsistent number of texture coordinates");
	}
}

template<class T>
void writeVector(std::ofstream& outfile, const std::string& type, const T& v)
{
	outfile << type << " ";
	for(int i=0; i<T::Dimension-1; i++)
	{
		outfile << v.data[i] << " ";
	}
	outfile << v.data[T::Dimension-1];
	outfile << std::endl;
}

void writeVertexIndex(std::ofstream& outfile, int index, bool hasNormals, bool hasTexCoord)
{
	index++;
	if(hasNormals && hasTexCoord)
	{
		outfile << index << "/" << index << "/" << index;
	}
	else if (hasNormals)
	{
		outfile << index << "/" << index;
	}
	else if (hasTexCoord)
	{
		outfile << index << "//" << index;
	}
}

void writeMaterialTexture(std::ofstream& outfile, const std::string& type, const std::string& textureFilename)
{
	if (!textureFilename.empty())
	{
		outfile << type << " " << textureFilename << std::endl;
	}
}

void writeObj(const std::string& filename, const std::string matFilename, const Mesh& mesh)
{
	// Open the file
	std::ofstream outfile;
	outfile.open(filename.c_str());
	if(!outfile.is_open())
	{
		throw std::runtime_error("Unable to open output mesh file: " + filename);
	}

	bool hasVertices = true;
	if(mesh.vertices.size()==0)
	{
		throw std::runtime_error("mesh contains no vertices");
	}

	bool hasNormals = false;
	if (mesh.normals.size() == mesh.vertices.size())
	{
		hasNormals = true;
	}
	else if (mesh.normals.size() > 0)
	{
		throw std::runtime_error("inconsistent number of normals");
	}
	bool hasTexCoord = false;
	if (mesh.texcoord.size() == mesh.vertices.size())
	{
		hasTexCoord = true;
	}
	else if (mesh.texcoord.size() > 0)
	{
		throw std::runtime_error("inconsistent number of vertex coordinates");
	}

	// Write material lib
	outfile << "mtllib " << matFilename << std::endl;

	// Write vertices
	outfile << "#vertices (" << mesh.vertices.size() << ")" << std::endl;
	for(size_t i=0;i<mesh.vertices.size();i++)
	{
		writeVector(outfile, "v", mesh.vertices[i]);
	}

	// Write normals
	if(hasNormals)
	{
		outfile << "#normals (" << mesh.normals.size() << ")" << std::endl;
		for(size_t i=0;i<mesh.normals.size();i++)
		{
			writeVector(outfile, "vn", mesh.normals[i]);
		}
	}
	else
	{
		outfile << "#normals not available" << std::endl;
	}
	
	// Write texture coordinates
	if(hasTexCoord)
	{
		outfile << "#texture coordinates (" << mesh.texcoord.size() << ")" << std::endl;
		for(size_t i=0;i<mesh.texcoord.size();i++)
		{
			writeVector(outfile, "vt", mesh.texcoord[i]);
		}
	}
	else
	{
		outfile << "#texture coordinates not available" << std::endl;
	}

	// Write components
	outfile << "#components (" << mesh.components.size() << ")" << std::endl;
	for(ComponentListType::const_iterator ic=mesh.components.begin(); ic!=mesh.components.end(); ++ic)
	{
		const MeshComponent& comp = *ic;
		// Component name
		outfile << "g " << comp.componentName << std::endl;
		// Component material
		if(!comp.materialName.empty())
		{
			outfile << "usemtl " << comp.materialName << std::endl;
		}
		outfile << "s " << 1 << std::endl;
		// Faces
		for(size_t i=0;i<comp.faces.size();++i)
		{
			outfile << "f ";
			writeVertexIndex(outfile, comp.faces[i].data[0], hasNormals, hasTexCoord);
			outfile << " ";
			writeVertexIndex(outfile, comp.faces[i].data[1], hasNormals, hasTexCoord);
			outfile << " ";
			writeVertexIndex(outfile, comp.faces[i].data[2], hasNormals, hasTexCoord);
			outfile << std::endl;
		}
	}
	outfile.close();

	// Write materials
	std::ofstream matfile;
	matfile.open(matFilename.c_str());
	if(!matfile.is_open())
	{
		throw std::runtime_error("Unable to open output material file: " + matFilename);
	}
	for(MaterialMapType::const_iterator im=mesh.materials.begin(); im!=mesh.materials.end(); ++im)
	{
		const std::string& name = im->first;
		const Material& mat = im->second;

		matfile << "#material" << std::endl;
		matfile << "newmtl " << name << std::endl;
		matfile << "illum " << mat.illuminationModel << std::endl;
		writeVector(matfile, "Ka", mat.colorAmbient);
		writeVector(matfile, "Kd", mat.colorDiffuse);
		writeVector(matfile, "Ks", mat.colorSpecular);
		writeVector(matfile, "Ke", mat.colorEmissive);
		matfile << "Ns " << mat.shininess << std::endl;
		matfile << "d " << mat.transparency << std::endl;
		writeMaterialTexture(matfile, "map_Ka",   mat.textureAmbient);
		writeMaterialTexture(matfile, "map_Kd",   mat.textureDiffuse);
		writeMaterialTexture(matfile, "map_Ks",   mat.textureSpecular);
		writeMaterialTexture(matfile, "map_Ke",   mat.textureEmissive);
		writeMaterialTexture(matfile, "map_bump", mat.textureBump);
		writeMaterialTexture(matfile, "map_d",    mat.textureTransparency);
		matfile << std::endl;
	}
	matfile.close();
}

#include "bakeObj.h"
#include "parser.h"
#include "packer.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "usage: bakeObj input-file" << std::endl;
		std::cout << "parameters:" << std::endl;
		std::cout << "  input-file: filename of the input obj file" << std::endl;
		return 0;
	}

	try
	{
		std::string filename_in(argv[1]);
		std::string filename_out(filename_in + ".baked.obj");
		std::string filename_mat(filename_in + ".baked.mtl");
		Mesh mesh_in;
		Mesh mesh_out;

		// Read and parse input mesh
		std::cout << "reading " << filename_in << "...";
		loadObj(filename_in, mesh_in);
		std::cout << " done." << std::endl;

		// Build texture atlas
		std::cout << "baking " << "...";
		packTextures(mesh_in, mesh_out);
		std::cout << " done." << std::endl;

		// Write output mesh
		std::cout << "writing " << filename_out << "...";
		writeObj(filename_out, filename_mat, mesh_out);
		std::cout << " done." << std::endl;

		return 0;
	}
	catch(std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl;
		return -1;
	}
}

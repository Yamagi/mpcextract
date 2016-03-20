#include <cinttypes>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <sys/stat.h>

namespace mpcextract
{
	/*
	 * Possible exit codes.
	 */
	enum class Exit : int
	{
		OK = 0,
		ERR,
		FTYPE,
		OPEN,
		READ,
		STAT,
		WRITE
	};

	/*
	 * Our exception class. We only need one.
	 */
	class Exception : std::exception
	{
	private:
		// Explanantory string.
		const std::string msg;

		// Value of errno.
		const int errnum;

		// Requested exit code.
		const Exit exitcode;

	public:
		/*
		 * Constructor.
		 *
		 * @param msg		Explanatory string.
		 * @param errnum	Value of errno.
		 * @param exitcode	Code to exit with.
		 */
		Exception(std::string _msg, int _errnum, Exit _exitcode) : msg(_msg), errnum(_errnum), exitcode(_exitcode)
		{
		}

		/*
		 * Copy constructor.
		 *
		 * @param	Other Object to copy from.
		 */
		Exception(const Exception &other) : msg(other.msg), errnum(other.errnum), exitcode(other.exitcode)
		{
		}

		/*
		 * Destructor.
		 */
		~Exception()
		{
		}

		/*
		 * Returns the value of 'errno'.
		 *
		 * @return	Value of 'errno'.
		 */
		const int getErrnum() const noexcept
		{
			return errnum;
		}

		/*
		 * Returns the string corresponding to 'errno'.
		 *
		 * @return	String corresponding to 'errno'.
		 */
		const std::string getErrstr() const noexcept
		{
			return std::string(strerror(errnum));
		}

		/*
		 * Returns the requested exit code.
		 *
		 * @return	Returns the requested exit code.
		 */
		const Exit getExitcode() const noexcept
		{
			return exitcode;
		}

		/*
		 * Returns the explanantory string.
		 *
		 * @return	Returns the explanatory string.
		 */
		virtual const char *what() const noexcept
		{
			return msg.c_str();
		}
	};

// ---------------------------------------------------------------------

	/*
	 * MPC file header.
	 *
	 * Byte 0 to 3:  Signature.
	 * Byte 4 to 7:  Directory offset.
	 * Byte 8 to 11: Unknown.
	 */
	struct MPCHeader
	{
		char signature[4];   // 'MPUC' on little endian.
		uint32_t dir_offset; // Offset from end of header to start of directory in bytes.
		uint32_t unknown;    // Unknown field, maybe a kind of parity bit.
	};

	/*
	 * Directory entry:
	 *
	 * Byte 0 to 63:  Filename.
	 * Byte 64 to 67: Fileposition.
	 * Byte 68 to 71: Filelength.
	 * Byte 72 to 75: Unknown.
	 * Byte 76 to 79: Unknown.
	 */
	struct MPCDirEntry
	{
		char name[64];     // Filename.
		uint32_t offset;   // Fileposition from start of MPC file in byte.
		uint32_t length;   // Filelength in byte.
		uint32_t unknown1; // Unknown field, seems to be the same as byte 68 to 71.
		uint32_t unknown2; // Unknown field, maybe a kind of parity bit.
	};

	class MPCFile
	{
	private:
		// MPC file directory
		std::vector<MPCDirEntry> dir;

		// MPC file object
		std::ifstream file;

		// MPC file header.
		MPCHeader header;

		// Number of files in the directory.
		uint32_t num_files;

	public:
		/*
		 * Constructor.
		 *
		 * Opens the file at _path, verifies that it is a MPC file and
		 * parses it's header and directory in instance variables.
		 *
		 * @param path	Path to the file to be opened.
		 */
		MPCFile(const std::string &path)
		{
			struct stat sb;

			if ((stat(path.c_str(), &sb)) != 0)
			{
				throw Exception{std::string("Couldn't stat ") + path, errno, Exit::STAT};
			}

			file.open(path, std::ios::in | std::ios::binary);

			if (!file.is_open())
			{
				throw Exception{std::string("Couldn't open ") + path, errno, Exit::OPEN};
			}

			// Byte 0 to 3: File signature.
			file.read((char *) &header.signature, sizeof(header.signature));

			if (file.fail())
			{
				throw Exception{std::string("Couldn't read file signature from ") + path, errno, Exit::READ};
			}

			if (strncmp(header.signature, "MPCU", 4) != 0)
			{
				throw Exception{std::string("Not a MPCU file"), 0, Exit::FTYPE};
			}

			// Byte 4 to 7: Directory offset (from end of header).
			file.read((char *) &header.dir_offset, sizeof(header.dir_offset));

			// Byte 8 to 11: Unknown field, maybe a kind of parity bit.
			file.read((char *) &header.unknown, sizeof(header.unknown));

			if (file.fail())
			{
				throw Exception{std::string("Couldn't read file header from ") + path, errno, Exit::READ};
			}

			file.seekg(header.dir_offset);

			// Byte header.dir_offset to header.dir_offset + 4: Number of files in directory.
			file.read((char *) &num_files, sizeof(num_files));

			for (int i = 0; i < num_files; i++)
			{
				MPCDirEntry entry;

				// Byte 0 to 63: Filename
				file.read((char *) &entry.name, sizeof(entry.name));

				if (file.fail())
				{
					throw Exception{"Couldn't read file directory", 0, Exit::READ};
				}

				// Byte 64 to 67: Fileposition from start of MPC file in byte.
				file.read((char *) &entry.offset, sizeof(entry.offset));

				if (file.fail())
				{
					throw Exception{"Couldn't read file directory", 0, Exit::READ};
				}

				// Byte 68 to 71: Filelength in byte.
				file.read((char *) &entry.length, sizeof(entry.length));

				if (file.fail())
				{
					throw Exception{"Couldn't read file directory", 0, Exit::READ};
				}

				// Byte 72 to 79: Unknown fields.
				file.read((char *) &entry.unknown1, sizeof(entry.unknown1));
				file.read((char *) &entry.unknown2, sizeof(entry.unknown2));

				if (file.fail())
				{
					throw Exception{"Couldn't read file directory", 0, Exit::READ};
				}

				dir.push_back(entry);
			}
		}

		/*
		 * Copy constrcutor.
		 *
		 * There's no need for a copy constructor.
		 *
		 * @param	Other Object to copy from.
		 */
		MPCFile(const MPCFile &other) = delete;

		/*
		 * Destructor.
		 */
		~MPCFile()
		{
		}

		/*
		 * Extracts one file.
		 *
		 * @param entry	Directory entry of the file to be extracted.
		 */
		void extractFile(MPCDirEntry &entry)
		{
			std::ofstream outfile(entry.name, std::ios::out | std::ios::binary);

			if (!outfile.is_open())
			{
				throw Exception(std::string("Couldn't open output file ") + entry.name, 0, Exit::WRITE);
			}

			file.seekg(entry.offset);

			int written = 0;
			char buf[128];

			while (true)
			{
				if ((entry.length - written) < 128)
				{
					file.read((char *) &buf, entry.length - written);

					if (file.fail())
					{
						throw Exception{"Couldn't read file contents", 0, Exit::READ};
					}

					outfile.write(buf, entry.length - written);

					if (outfile.fail())
					{
						throw Exception{std::string("Couldn't write output file ") + entry.name, 0, Exit::WRITE};
					}

					break;
				}
				else
				{
					file.read((char *) &buf, 128);

					if (file.fail())
					{
						throw Exception{"Couldn't read file contents", 0, Exit::READ};
					}

					outfile.write(buf, 128);

					if (outfile.fail())
					{
						throw Exception{std::string("Couldn't write output file ") + entry.name, 0, Exit::WRITE};
					}

					written += 128;
				}
			}
		}

		/*
		 * Returns the directory.
		 *
		 * @return	Directory of the MPC file.
		 */
		std::vector<MPCDirEntry> const getDirectory() const noexcept
		{
			return dir;
		}

		/*
		 * Returns the number of files in the directory.
		 *
		 * @return	Number of files in the directory.
		 */
		uint32_t const getNumberOfDirEntries() const noexcept
		{
			return num_files;
		}
	};

// ---------------------------------------------------------------------

	/*
	 * Prints a message to stderr and exists.
	 *
	 * @param e	Exception object to gather data from.
	 */
	void error(const mpcextract::Exception e)
	{
		std::cerr << e.what();

		if (e.getErrnum() != 0)
		{
			std::cerr << ": " << e.getErrstr();
		}

		std::cerr << std::endl;
		exit((int) e.getExitcode());
	}

	/*
	 * Prints a small help message to stderr and exists.
	 */
	void usage()
	{
		std::cerr << "Usage: mpcextract input.mpc" << std::endl;
		std::cerr << std::endl;
		std::cerr << "Extracts an Monkeystone games MPC file into the current directory" << std::endl;
		exit((int) mpcextract::Exit::ERR);
	}
}

/*
 * mpcextract a a simple tool to extract the contents of an Monkeystone MPC file
 * (for example from Hyperspace Delivery Boy) into the current working directory.
 */
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		mpcextract::usage();
	}

	try
	{
		mpcextract::MPCFile mpcfile(argv[1]);

		std::cout << "Extracting " << mpcfile.getNumberOfDirEntries() << " files:" << std::endl;

		for (auto entry : mpcfile.getDirectory())
		{
			std::cout << " - " << entry.name << ": ";
			mpcfile.extractFile(entry);
			std::cout << "OK" << std::endl;
		}

		std::cout << "Done" << std::endl;
	}
	catch (mpcextract::Exception e)
	{
		mpcextract::error(e);
	}

	return (int)mpcextract::Exit::OK;
}

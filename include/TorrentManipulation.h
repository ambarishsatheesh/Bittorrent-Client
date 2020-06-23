#ifndef TORRENTMANIPULATION_H
#define TORRENTMANIPULATION_H

#include "Torrent.h"
#include "Utility.h"
#include "encodeVisitor.h"
#include "sha1.h"


namespace Bittorrent
{

	using namespace utility;

	//need to declare inline for each function?

	namespace torrentManipulation
	{
		//forward declare
        std::string encode(const value& torrent);
        Torrent toTorrentObj(const char* fullFilePath,
            const valueDictionary& torrentDict);
        value toBencodingObj(Torrent& torrent);
        void createNewTorrent(std::string fileName, const char* path,
            std::string targetPath, bool isPrivate, const std::string& comment,
            std::vector<std::string> trackerList);
        std::vector<byte> read(Torrent& torrent, long long start, long long length);
        void write(Torrent& torrent, long long start, std::vector<byte>& buffer);
        std::vector<byte> readPiece(Torrent& torrent, int piece);
        std::vector<byte> readBlock(Torrent& torrent, int piece, long long offset,
            long long length);
        void writeBlock(Torrent& torrent, int piece, int block,
            std::vector<byte>& buffer);
        void verify(Torrent& torrent, int piece);
        std::vector<byte> getHash(Torrent& torrent, int piece);


		//encode torrent object
        inline std::string encode(const value& torrent)
		{
			return boost::apply_visitor(encodeVisitor(), torrent);
		}

		//create complete torrent object from bencoded data
        inline Torrent toTorrentObj(const char* fullFilePath,
			const valueDictionary& torrentDict)
		{
			if (torrentDict.empty())
			{
				throw std::invalid_argument("Error: not a torrent file!");
			}
			if (!torrentDict.count("info"))
			{
				throw std::invalid_argument("Error: missing info section in torrent!");
			}

            //create torrent
            Torrent newTorrent;
			//fill file list data
            newTorrent.setFileList(torrentDict);
            //fill general data
            newTorrent.generalData.torrentToGeneralData(fullFilePath, torrentDict);
            //fill pieces data
            newTorrent.piecesData.torrentToPiecesData(newTorrent.fileList, torrentDict);
            newTorrent.hashesData.torrentToHashesData(torrentDict);

            //encode and create info section for use in new torrent
            valueDictionary bencodingObjInfo;
            //if multi files, use input filename as name
            bencodingObjInfo = newTorrent.filesToDictionary(bencodingObjInfo);
            bencodingObjInfo.emplace("name", newTorrent.generalData.fileName);
            bencodingObjInfo =
                newTorrent.piecesData.piecesDataToDictionary(bencodingObjInfo);
            newTorrent.hashesData.torrentToHashesData(bencodingObjInfo);

			return newTorrent;
		}


		//create complete torrent object (pre-processing for encoding)
        inline value toBencodingObj(Torrent& torrent)
		{
			valueDictionary bencodingObj;
			valueDictionary bencodingObjInfo;

			torrent.generalData.generalDataToDictionary(bencodingObj);

			//create info section
			bencodingObjInfo = torrent.filesToDictionary(bencodingObjInfo);
			//if multi files, use input filename as name
			bencodingObjInfo.emplace("name", torrent.generalData.fileName);
			bencodingObjInfo = torrent.piecesData.piecesDataToDictionary(bencodingObjInfo);
			bencodingObj.emplace("info", bencodingObjInfo);


			return bencodingObj;
		}


		//create torrent with default empty tracker list and comment
        inline void createNewTorrent(std::string fileName, const char* path,
			std::string targetPath, bool isPrivate, const std::string& comment,
			std::vector<std::string> trackerList)
		{
            Torrent createdTorrent;

			// General data
			createdTorrent.generalData.fileName = fileName;
			createdTorrent.generalData.downloadDirectory = getFileDirectory(path);
			createdTorrent.generalData.comment = comment;
			createdTorrent.generalData.createdBy = "myBittorrent";
			createdTorrent.generalData.creationDate =
				boost::posix_time::second_clock::local_time();
			createdTorrent.generalData.encoding = "UTF-8";
			createdTorrent.generalData.isPrivate = isPrivate;

			//tracker data
			std::vector<trackerObj> trackers;
			for (auto singleTracker : trackerList)
			{
				trackerObj trkObj;
				trkObj.trackerAddress = singleTracker;
				trackers.push_back(trkObj);
			}
			createdTorrent.generalData.trackerList = trackers;

			// File data 
			std::vector<fileObj> files;
			fileObj resObj;

			long long runningOffset = 0;

			// query file/folder existence and sizes
			if (boost::filesystem::exists(path))
			{
				if (boost::filesystem::is_regular_file(path))
				{
					resObj.filePath = getFileNameAndExtension(path);
					resObj.fileSize = boost::filesystem::file_size(path);
					resObj.fileOffset = runningOffset;
					resObj.setReadableFileSize();
					files.push_back(resObj);
				}
				else if (boost::filesystem::is_directory(path))
				{
					boost::filesystem::recursive_directory_iterator end_itr;

					// cycle through the directory and push file path and size into fileObj
					for (boost::filesystem::recursive_directory_iterator itr(path);
						itr != end_itr; ++itr)
					{
						if (is_regular_file(itr->path())) {
							//remove parent directory
							std::string curFilePath = itr->path().string();
							const size_t pos = curFilePath.find(path) + strlen(path);

							long long size = boost::filesystem::file_size(curFilePath);
							resObj.fileSize = size;
							resObj.filePath = curFilePath.substr(pos);
							resObj.fileOffset = runningOffset;
							//add to running size total
							runningOffset += size;
							resObj.setReadableFileSize();
							files.push_back(resObj);
						}
					}
				}
				else
				{
					throw std::invalid_argument
					("Provided path is neither a file or directory!");
				}
			}
			else
			{
				throw std::invalid_argument("Provided path does not exist!");
			}

			createdTorrent.fileList = files;

			// Piece data
			long long totalSize = 0;

			for (auto file : createdTorrent.fileList)
			{
				totalSize += file.fileSize;
			}

			createdTorrent.piecesData.totalSize = totalSize;
			createdTorrent.piecesData.pieceSize = recommendedPieceSize(totalSize);

            const auto count = static_cast<int>(std::ceil(
                static_cast<float>(createdTorrent.piecesData.totalSize) /
                createdTorrent.piecesData.pieceSize));

			//resize vectors for processing later
			createdTorrent.piecesData.pieces.resize(count);
			createdTorrent.statusData.isPieceVerified.resize(count);
			createdTorrent.statusData.isBlockAcquired.resize(count);

			//set pieceCount
			createdTorrent.piecesData.pieceCount = count;
			const int pieceCount = createdTorrent.piecesData.pieceCount;

			for (size_t i = 0; i < pieceCount; ++i)
			{
				createdTorrent.statusData.isBlockAcquired.at(i).resize(
					createdTorrent.piecesData.setBlockCount(i));
			}

			for (size_t i = 0; i < pieceCount; ++i)
			{
				createdTorrent.piecesData.pieces.at(i) = getHash(createdTorrent, i);
			}

			//***************************
			//create infohash
			//***************************
			//encode and create info section for use in new torrent
			valueDictionary bencodingObjInfo;
			bencodingObjInfo.emplace("name", createdTorrent.generalData.fileName);
			bencodingObjInfo = createdTorrent.filesToDictionary(bencodingObjInfo);
			bencodingObjInfo = createdTorrent.piecesData.piecesDataToDictionary(bencodingObjInfo);
			createdTorrent.hashesData.torrentToHashesData(bencodingObjInfo);

			//create object for encoding
			value tempObj = toBencodingObj(createdTorrent);
			//encode and save
			saveToFile(targetPath, encode(tempObj));
		}


		//load bytes from each file into a buffer
        inline std::vector<byte> read(Torrent& torrent, long long start, long long length)
		{
			const auto end = start + length;
			std::vector<byte> buffer;
			buffer.resize(length);

			for (size_t i = 0; i < torrent.fileList.size(); ++i)
			{
				auto file = torrent.fileList.at(i);

				if ((start < file.fileOffset && end < file.fileOffset) ||
					(start > file.fileOffset + file.fileSize
						&& end > file.fileOffset + file.fileSize))
				{
					continue;
				}

				std::string filePath = torrent.generalData.downloadDirectory +
					file.filePath;

				if (!boost::filesystem::exists(filePath))
				{
					buffer.clear();
					return buffer;
					/*throw std::invalid_argument("The file path \"" + filePath +
						"\" does not exist!");*/
				}

				const auto fStart = std::max(static_cast<long long>(0),
					start - file.fileOffset);
				const auto fEnd = std::min(end - static_cast<long long>(file.fileOffset),
					file.fileSize);
				const auto fLength = fEnd - fStart;
				const auto bStart = std::max(static_cast<long long>(0),
					file.fileOffset - start);

                std::ifstream stream(filePath, std::ios::binary);
				//set position of next character to be extracted to fStart
				stream.seekg(fStart, std::ifstream::beg);
				//set offset in buffer at which to begin storing read data
				auto bufferStart = &buffer[bStart];
				//extract fLength characters from stream (starting at fStart) 
				//into buffer (starting at bufferStart)
				stream.read(reinterpret_cast<char*>(bufferStart), fLength);
			}

			return buffer;
		}

		//slice chunks out of byte buffer and write to correct position in each file
        inline void write(Torrent& torrent, long long start, std::vector<byte>& buffer)
		{
			const long long end = start + buffer.size();

			for (size_t i = 0; i < torrent.fileList.size(); ++i)
			{
				auto file = torrent.fileList.at(i);

				if ((start < file.fileOffset && end < file.fileOffset) ||
					(start > file.fileOffset + file.fileSize
						&& end > file.fileOffset + file.fileSize))
				{
					continue;
				}

				std::string filePath = torrent.generalData.downloadDirectory +
					file.filePath;

				const char* cstrPath = filePath.c_str();

				std::string dir = getFileDirectory(cstrPath);
				if (!boost::filesystem::is_directory(dir))
				{
					boost::filesystem::create_directory(dir);
				}

				const auto fStart = std::max(static_cast<long long>(0),
					start - file.fileOffset);
				const auto fEnd = std::min(end - static_cast<long long>(file.fileOffset),
					file.fileSize);
				const auto fLength = fEnd - fStart;
				const auto bStart = std::max(static_cast<long long>(0),
					file.fileOffset - start);

                std::ifstream stream(filePath, std::ios::binary);
				//set position of next character to be extracted to fStart
				stream.seekg(fStart, std::ifstream::beg);
				//set offset in buffer at which to begin storing read data
				auto bufferStart = &buffer.at(bStart);
				//extract fLength characters from stream (starting at fStart) 
				//into buffer (starting at bufferStart)
				stream.read(reinterpret_cast<char*>(bufferStart), fLength);
			}
		}

        inline std::vector<byte> readPiece(Torrent& torrent, int piece)
		{
			return read(torrent, piece * torrent.piecesData.pieceSize,
				torrent.piecesData.setPieceSize(piece));
		}

        inline std::vector<byte> readBlock(Torrent& torrent, int piece, long long offset,
			long long length)
		{
			return read(torrent, (piece * torrent.piecesData.pieceSize) + offset, length);
		}

        inline void writeBlock(Torrent& torrent, int piece, int block, std::vector<byte>& buffer)
		{
			write(torrent, (piece * torrent.piecesData.pieceSize) +
				(block * torrent.piecesData.blockSize), buffer);
			torrent.statusData.isBlockAcquired.at(piece).at(block) = true;
			verify(torrent, piece);
		}

        inline void verify(Torrent& torrent, int piece)
		{
			std::vector<byte> hash = getHash(torrent, piece);

			//check if piece hash info matches currently generated hash
            bool isVerified = (!hash.empty() && hash == torrent.piecesData.pieces.at(piece));

			//if piece passes verification, fill relevant vectors
			if (isVerified)
			{
				torrent.statusData.isPieceVerified.at(piece) = true;

				for (size_t i = 0; i <
					(torrent.statusData.isBlockAcquired.at(piece)).size(); ++i)
				{
					torrent.statusData.isBlockAcquired[piece][i] = true;
				}

				//if slots are connected to signal, call slots
				//need to implement properly after slot function is defined
				if (!torrent.piecesData.pieceVerifiedSig->empty())
				{
					torrent.piecesData.pieceVerifiedSig->operator()(piece);
				}

				return;
			}

			//check if all the blocks in a piece have been acquired
			//if they have (and piece fails verification above), 
			//reload entire piece
			torrent.statusData.isPieceVerified.at(piece) = false;
			if (
				std::all_of(
					torrent.statusData.isBlockAcquired.at(piece).begin(), 
					torrent.statusData.isBlockAcquired.at(piece).end(),
					[](bool v) {return v; }))
			{
				for (size_t i = 0; 
					i < torrent.statusData.isBlockAcquired.at(piece).size(); ++i)
				{
					torrent.statusData.isBlockAcquired[piece][i] = false;
				}
			}
		}

        inline std::vector<byte> getHash(Torrent& torrent, int piece)
		{
			std::vector<byte> data = readPiece(torrent, piece);
			byte* dataArr = &data[0];

			if (data.empty())
			{
				return data;
			}

			//calculate hash
			SHA1 sha1;
			auto hashData = sha1(dataArr, data.size());

			//return sha1 hash of data
			return hashData;
		}
	}
}

#endif // TORRENTMANIPULATION_H

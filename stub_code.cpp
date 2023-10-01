#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include <cmath>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 512

// Function to convert decimal to binary char..
char decToBinary(int n) {
    return static_cast<char>(n);
}
//function to convert binary char to dec
int binaryToDec(char c) {
    return static_cast<int>(c);
}


class fsInode {
    int fileSize;
    int block_in_use;
    int directBlock1;
    int directBlock2;
    int directBlock3;
    int singleInDirect;
    int doubleInDirect;
    int block_size;

public:
    int lastBlock;
    int freeCharInLastBlock;
    int maxSizeOfFile;
    int indexInSingle;
    int indexOfExDouble;
    int indexOfInDouble;
    int numOfCurPtrBlock;

    fsInode(int _block_size) {
        fileSize = 0;
        block_in_use = 0;
        block_size = _block_size;
        directBlock1 = -1;
        directBlock2 = -1;
        directBlock3 = -1;
        singleInDirect = -1;
        doubleInDirect = -1;

        lastBlock=-1;
        freeCharInLastBlock=0;
        maxSizeOfFile=(3+block_size+block_size*block_size)*block_size;
        indexInSingle=0;
        indexOfExDouble=0;
        indexOfInDouble=0;
        numOfCurPtrBlock=-1;
    }

    [[nodiscard]] int placeInFile() const{//function to check how much place in file
        return maxSizeOfFile - fileSize;
    }

    //getters and setters for each attribute

    [[nodiscard]] int getFileSize() const{
        return  fileSize;
    }
    void setFileSize(int size){
        fileSize=size;
    }
    [[nodiscard]] int getDirectBlock1() const {
        return directBlock1;
    }
    void setDirectBlock1(int _directBlock1) {
        fsInode::directBlock1 = _directBlock1;
    }
    [[nodiscard]] int getDirectBlock2() const {
        return directBlock2;
    }
    void setDirectBlock2(int _directBlock2) {
        fsInode::directBlock2 = _directBlock2;
    }
    [[nodiscard]] int getDirectBlock3() const {
        return directBlock3;
    }
    void setDirectBlock3(int _directBlock3) {
        fsInode::directBlock3 = _directBlock3;
    }
    [[nodiscard]] int getSingleInDirect() const {
        return singleInDirect;
    }
    void setSingleInDirect(int _singleInDirect) {
        fsInode::singleInDirect = _singleInDirect;
    }
    [[nodiscard]] int getDoubleInDirect() const {
        return doubleInDirect;
    }
    void setDoubleInDirect(int _doubleInDirect) {
        fsInode::doubleInDirect = _doubleInDirect;
    }
    [[nodiscard]] int getBlockInUse() const {
        return block_in_use;
    }
    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;
    }

};

class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse; //if it is false, means that the fd does not really exist.

public:
    FileDescriptor(string FileName, fsInode* fsi) {//only open file
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }
    //getters and setters for each attribute
    string getFileName() {
        return file.first;
    }
    void setFileName(string newFileName){
        file.first = newFileName;
    }
    fsInode* getInode() {
        return file.second;
    }
    int GetFileSize() {
        return file.second->getFileSize();
    }
    bool isInUse() {
        return (inUse);
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.)
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures,
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ;

    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector< FileDescriptor > OpenFileDescriptors;

    int block_size;
    int numOfFreeBlocks;
    vector< fsInode* > delInode;
public:

    fsDisk() {//initialize the disk with null
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        assert(sim_disk_fd);
        init();
        BitVector = nullptr;
    }

    void init(){//initialize the disk with null
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            assert(ret_val == 0);
            ret_val = int(fwrite( "\0" ,  1 , 1, sim_disk_fd ));
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }

    void listAll() {//prints all fd and the disk's content
        if(!is_formated){
            cout << "ERR" << endl; return;
        }
        int i = 0;
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: "
                 << it->isInUse() << " file Size: " << it->GetFileSize() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            assert(ret_val == 0);
            ret_val = int(fread(  &bufy , 1 , 1, sim_disk_fd ));
            assert(ret_val != -1);
            cout << bufy;
        }
        cout << "'" << endl;
    }
    //quick format
    void fsFormat( int blockSize = 4) {
        if(blockSize<2 || blockSize>DISK_SIZE){//illegal input.
            cout << "ERR" << endl;
            return;
        }
        delete [] BitVector;
        BitVectorSize=(DISK_SIZE)/blockSize;
        BitVector=new int[BitVectorSize];
        for (int i = 0; i < BitVectorSize; ++i) {
            //Quick format - marking that the entire disk is free, but in practice nothing is deleted from the disk
            BitVector[i]=0;
        }
        block_size=blockSize;
        numOfFreeBlocks=BitVectorSize;
        clearMD();
        OpenFileDescriptors.clear();
        init();
        is_formated = true;
    }

    int CreateFile(string fileName) {
        if (!is_formated || MainDir.count(fileName) > 0) {//the disk is not formatted or this file name already exist
            cout << "ERR" << endl;
            return -1;
        }
        auto* fsI = new fsInode(block_size);
        auto* fd = new FileDescriptor(fileName,fsI);
        MainDir[fileName]=fsI;
        int temp = nextFd(fd);
        delete fd;
        return temp;
    }

    int OpenFile(string fileName){
        if(!is_formated || getFd(fileName) != -1 || MainDir.count(fileName) == 0) {
            cout << "ERR" << endl;
            return -1;//the disk is not formatted or file already open or file does not exist
        }
        auto* fd = new FileDescriptor(fileName,MainDir[fileName]);
        int temp = nextFd(fd);
        delete fd;
        return temp;
    }
    int nextFd(FileDescriptor* fd){//find the first available fd and use is and returns the number of fd
        int i=0;
        for (; i < OpenFileDescriptors.size(); ++i) {
            if (!OpenFileDescriptors[i].isInUse()) {
                OpenFileDescriptors[i]=*fd;
                break;
            }
        }
        if(i==OpenFileDescriptors.size())
            OpenFileDescriptors.push_back(*fd);
        return i;
    }

    string CloseFile(int fd) {
        if(!is_formated || fd<0 || fd>=OpenFileDescriptors.size() ) {//the disk is not formatted or illegal fd
            cout << "ERR" << endl;
            return "-1";
        }
        FileDescriptor* it=&OpenFileDescriptors[fd];
        if(!it->isInUse()) {//file already close
            cout << "ERR" << endl;
            return "-1";
        }
        it->setInUse(false);
        return it->getFileName();
    }

    int getFd(string fileName){//receives file name and return the fd, if the file close then returns -1
        int i=-1;
        for (FileDescriptor& it : OpenFileDescriptors) {
            i++;
            if(it.getFileName()==fileName){
                if(!it.isInUse())
                    continue;
                return i;
            }
        }
        return -1;
    }

    int WriteToFile(int fd, char *buf, int len ) {
        if(!is_formated || fd<0 || fd>=OpenFileDescriptors.size() || len<0) {//The disk is not formatted or illegal input.
            cout << "ERR" << endl; return -1;
        }
        FileDescriptor it = OpenFileDescriptors[fd];
        if(!it.isInUse()) {//file is closed
            cout << "ERR" << endl; return -1;
        }
        fsInode* inode = it.getInode();
        int placeInFile = inode->placeInFile();
        if(placeInFile<len) {//no place in file
            if(placeInFile==0){
                cout << "ERR" << endl; return -1;
            }
            len = placeInFile;
        }
        int indexInBuf=0; bool is_written = false;
        if(inode->freeCharInLastBlock!=0){//In case there is a place in the last block, then fill it
            int offset=(inode->lastBlock)*block_size + (block_size-inode->freeCharInLastBlock);
            if(len<inode->freeCharInLastBlock){
                writeFromOffset(buf,offset,indexInBuf,len);
                inode->setFileSize(inode->getFileSize() + len);
                inode->freeCharInLastBlock=inode->freeCharInLastBlock-len;
                return 1;
            }
            writeFromOffset(buf,offset,indexInBuf,inode->freeCharInLastBlock);
            indexInBuf=inode->freeCharInLastBlock;
            inode->setFileSize(inode->getFileSize() + inode->freeCharInLastBlock);
            inode->freeCharInLastBlock=0;
            is_written = true;
        }

        int numOfBlock, offset,sizeToWrite=block_size; char c[1];
        while(numOfFreeBlocks>0 && indexInBuf<len){
            //each time save pointer in file to a new block and write block_size characters to the disk (besides the last time)
            numOfBlock=newBlock();
            inode->setBlockInUse(inode->getBlockInUse()+1);
            if(inode->getDirectBlock1()==-1)
                inode->setDirectBlock1(numOfBlock);
            else if(inode->getDirectBlock2()==-1)
                inode->setDirectBlock2(numOfBlock);
            else if(inode->getDirectBlock3()==-1)
                inode->setDirectBlock3(numOfBlock);
            else if(inode->getSingleInDirect()==-1) {//single indirect
                if(numOfFreeBlocks==0){
                    inode->setBlockInUse(inode->getBlockInUse()-1);
                    BitVector[numOfBlock]=0; numOfFreeBlocks++; break;
                }
                inode->setSingleInDirect(numOfBlock);
                numOfBlock=newBlock();//the first block whose pointer's address stored in the block of single indirect
                inode->setBlockInUse(inode->getBlockInUse()+1);
                c[0] = decToBinary(numOfBlock);
                //storing its address on disk
                writeFromOffset(c,inode->getSingleInDirect()*block_size,0,1);
                inode->indexInSingle++;
            } else if( inode->indexInSingle<block_size){//single indirect - the rest of the blocks
                c[0] = decToBinary(numOfBlock);
                offset=inode->getSingleInDirect()*block_size+inode->indexInSingle;
                writeFromOffset(c,offset,0,1);
                inode->indexInSingle++;
            }else if(inode->getDoubleInDirect()==-1){//double indirect
                if(numOfFreeBlocks<2){
                    inode->setBlockInUse(inode->getBlockInUse()-1);
                    BitVector[numOfBlock]=0; numOfFreeBlocks++; break;
                }
                inode->setDoubleInDirect(numOfBlock);
                numOfBlock=newBlock();
                c[0] = decToBinary(numOfBlock);
                writeFromOffset(c,inode->getDoubleInDirect()*block_size,0,1);
                inode->numOfCurPtrBlock=numOfBlock;
                numOfBlock=newBlock();
                c[0] = decToBinary(numOfBlock);
                writeFromOffset(c,inode->numOfCurPtrBlock*block_size,0,1);
                inode->indexOfInDouble++;
                inode->setBlockInUse(inode->getBlockInUse()+2);
            } else if(inode->indexOfInDouble<block_size){//double indirect - the innermost blocks
                c[0] = decToBinary(numOfBlock);
                offset=inode->numOfCurPtrBlock*block_size+inode->indexOfInDouble;
                writeFromOffset(c,offset,0,1);
                inode->indexOfInDouble++;
            }else{//double indirect - the inner blocks
                if(numOfFreeBlocks==0){
                    inode->setBlockInUse(inode->getBlockInUse()-1);
                    BitVector[numOfBlock]=0; numOfFreeBlocks++; break;
                }
                inode->indexOfExDouble++;
                c[0] = decToBinary(numOfBlock);
                writeFromOffset(c,inode->getDoubleInDirect()*block_size + inode->indexOfExDouble,0,1);
                inode->numOfCurPtrBlock=numOfBlock;
                numOfBlock=newBlock();
                inode->setBlockInUse(inode->getBlockInUse()+1);
                c[0] = decToBinary(numOfBlock);
                writeFromOffset(c,inode->numOfCurPtrBlock*block_size,0,1);
                inode->indexOfInDouble=1;
            }
            if(len-indexInBuf<block_size) {//if in the last block we need to write less than block_size characters
                sizeToWrite = len - indexInBuf;
                inode->freeCharInLastBlock = block_size - (len - indexInBuf);
            }
            writeFromOffset(buf,numOfBlock*block_size,indexInBuf,sizeToWrite);//writing data to disk (not pointers)
            inode->lastBlock=numOfBlock;
            indexInBuf+=sizeToWrite;
            inode->setFileSize(inode->getFileSize() + sizeToWrite);
            is_written = true;
        }
        if(!is_written && len!=0){
            cout << "ERR" << endl; return -1;
        }
        return 1;
    }

    void writeFromOffset(char *buf,int offset, int indexInBuf,int sizeToWrite){//writes data to disk from the received offset
        int ret_val = fseek ( sim_disk_fd , offset , SEEK_SET );
        assert(ret_val == 0);
        size_t val = fwrite( &buf[indexInBuf], sizeof(char), sizeToWrite ,sim_disk_fd);
        assert(val != -1);
        fflush(sim_disk_fd);
    }

    int newBlock(){//finds available block and returns its number
        for (int i = 0; i < BitVectorSize; ++i) {
            if(BitVector[i]==0){
                BitVector[i]=1;
                numOfFreeBlocks--;
                return i;
            }
        }
        return -1;
    }

    int DelFile( string FileName ) {
        if(!is_formated || MainDir.count(FileName) == 0 || getFd(FileName)!=-1) {
            cout << "ERR" << endl; return -1;//the disk is not formatted or file does not exist or file is open
        }
        fsInode* inode = MainDir[FileName];
        int numOfBlockToDel;
        for (int i = 0; i < inode->getBlockInUse(); ++i) {//marks that all the blocks of the file are available
            numOfBlockToDel = numOfBlockToRead(i,inode, true);
            BitVector[numOfBlockToDel]=0;
        }
        numOfFreeBlocks+=inode->getBlockInUse();
        delInode.push_back(inode);
        inode->setFileSize(0);
        MainDir.erase(FileName);
        for (FileDescriptor& it : OpenFileDescriptors) {
            if (it.getFileName() == FileName)
                it.setFileName("");
        }
        return 1;
    }

    int ReadFromFile(int fd, char *buf, int len ) {
        buf[0]='\0';
        if(!is_formated || fd<0 || fd>=OpenFileDescriptors.size() || len < 0) {//the disk is not formatted or illegal input
            cout << "ERR" << endl;
            return -1;
        }
        FileDescriptor it = OpenFileDescriptors[fd];
        if(!it.isInUse()) {// file is closed
            cout << "ERR" << endl;
            return -1;
        }
        if(len>it.GetFileSize()){//want to read more than allowed
            len=it.GetFileSize();
        }
        int indexInBuf=0,sizeToRead=block_size;
        for (int i=0; indexInBuf<len ; i++) {//each time reads block_size characters
            if(len-indexInBuf<block_size)
                sizeToRead=len-indexInBuf;
            int ret_val = fseek ( sim_disk_fd ,numOfBlockToRead(i,it.getInode(), false)*block_size, SEEK_SET );
            assert(ret_val==0);
            size_t val = fread( &buf[indexInBuf], sizeof(char), sizeToRead ,sim_disk_fd);
            assert(val!=-1);
            indexInBuf+=sizeToRead;
        }
        buf[len]='\0';
        return 1;
    }

    //this function receives number of block and read is content.
    //if isAll==true then reads also the content of blocks with numbers of other blocks
    int numOfBlockToRead(int i,fsInode* inode ,bool isAll){
        char c[1];
        if(i==0)
            return inode->getDirectBlock1();
        if(i==1)
            return inode->getDirectBlock2();
        if(i==2)
            return inode->getDirectBlock3();
        if(i==3 && isAll)
            return inode->getSingleInDirect();
        if(isAll)
            i--;
        if(i>2 && i<=2+block_size){//single indirect
            readFromOffset(c,inode->getSingleInDirect()*block_size+i-3,0,1);
            return binaryToDec(c[0]);
        }
        //double indirect
        if(isAll){//if we read pointers blocks
            if(i==3+block_size)
                return inode->getDoubleInDirect();
            i-=4+block_size;
            if(i%(block_size+1)==0) {//block of pointers
                i=i%block_size;
                readFromOffset(c, inode->getDoubleInDirect() * block_size+i, 0, 1);
                return binaryToDec(c[0]);
            }//the rest of the blocks
            int ex=i/(block_size+1);
            i--;
            int in=i%(block_size+1);
            readFromOffset(c,inode->getDoubleInDirect()*block_size+ex,0,1);
            readFromOffset(c, binaryToDec(c[0])*block_size+in,0,1);
            return binaryToDec(c[0]);

        }
        i-=3+block_size;
        int ex=i/block_size;
        int in=i%block_size;
        readFromOffset(c,inode->getDoubleInDirect()*block_size+ex,0,1);
        readFromOffset(c, binaryToDec(c[0])*block_size+in,0,1);
        return binaryToDec(c[0]);
    }

    void readFromOffset(char *buf,int offset, int indexInBuf,int sizeToRead){//reads data from disk from the received offset
        int ret_val = fseek ( sim_disk_fd , offset , SEEK_SET );
        assert(ret_val==0);
        size_t val = fread( &buf[indexInBuf], sizeof(char), sizeToRead ,sim_disk_fd);
        assert(val!=-1);
    }

    int GetFileSize(int fd) {
        if(!is_formated || fd<0 || fd>=OpenFileDescriptors.size()) {//the disk is not formatted or illegal fd
            cout << "ERR" << endl;
            return -1;
        }
        FileDescriptor it = OpenFileDescriptors[fd];
        if(!it.isInUse()) {// file is close
            cout << "ERR" << endl;
            return -1;
        }
        return it.GetFileSize();
    }

    int CopyFile(string srcFileName, string destFileName) {
        if(!is_formated || MainDir.count(srcFileName)==0 || getFd(srcFileName)!=-1
        || srcFileName==destFileName || MainDir[srcFileName]->getBlockInUse()>numOfFreeBlocks) {
            cout << "ERR" << endl;  //The disk is not formatted or file does not exist or srcFileName is open
            return -1;             // or Attempt to create a file with the same name as an existing file
        }                         //  or there is not enough place on the disk
        int fdOfNew;
        if(MainDir.count(destFileName)!=0){//If there is file with that name - we delete the old one and create new
            fdOfNew = getFd(destFileName);
            if(fdOfNew!=-1) {//if the file open
                cout << "ERR" << endl; return -1;
            }
            //file is close
            DelFile(destFileName);
        }
        int fdOfOld = OpenFile(srcFileName);
        fdOfNew = CreateFile(destFileName);
        char temp[DISK_SIZE];
        ReadFromFile(fdOfOld,temp, GetFileSize(fdOfOld));
        WriteToFile(fdOfNew,temp,GetFileSize(fdOfOld));
        CloseFile(fdOfNew);
        CloseFile(fdOfOld);
        return 1;
    }

    int RenameFile(string oldFileName, string newFileName) {
        if(!is_formated || getFd(oldFileName)!=-1 || MainDir.count(oldFileName) == 0
        || (MainDir.count(newFileName) > 0 && oldFileName!=newFileName)) {
            //The disk is not formatted or file is open or file does not exist or newFileName already exist.
            cout << "ERR" << endl;
            return -1;
        }
        fsInode* fsI=MainDir[oldFileName];
        MainDir[newFileName]=fsI;//creates new one with the same inode
        MainDir.erase(oldFileName);//erase the old one
        for (FileDescriptor& it : OpenFileDescriptors) {
            if (it.getFileName() == oldFileName)
                it.setFileName(newFileName);
        }
        return 1;
    }
    void clearMD(){
        for (const auto& entry : MainDir) {
            delete entry.second; // Delete the fsInode* pointers
        }
        MainDir.clear();
    }
    ~fsDisk(){
        delete [] BitVector;
        for (auto & i : delInode) {
            delete i;
        }
        clearMD();
        OpenFileDescriptors.clear();
        fclose(sim_disk_fd);
    }

};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    string fileName2;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while (1) {
        cin >> cmd_;
        switch (cmd_) {
            case 0:   // exit
                delete fs;
                exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs->ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 9:   // copy file
                cin >> fileName;
                cin >> fileName2;
                fs->CopyFile(fileName, fileName2);
                break;

            case 10:  // rename file
                cin >> fileName;
                cin >> fileName2;
                fs->RenameFile(fileName, fileName2);
                break;

            default:
                break;
        }
    }
}



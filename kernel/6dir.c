#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h> // 파일 시간 정보를 위해 추가

#define MAX_INODES 1024 // 최대 inode 개수
#define MAX_DATA_BLOCKS 4096 // 최대 데이터 블록 개수
#define BLOCK_SIZE 4096

typedef struct Inode {
    int fileSize; // 파일 크기
    time_t created; // 파일 생성 시간
    time_t modified; // 파일 수정 시간
    int linkCount; // 링크 수
    int dataBlockNumbers[10]; // 데이터 블록 번호 배열 (최대 10개)
    bool inUse; // inode 사용 여부 플래그
    // 여기에 더 많은 inode 관련 정보를 추가할 수 있습니다.
} Inode;

Inode inodeTable[MAX_INODES];
char dataBlocks[MAX_DATA_BLOCKS][BLOCK_SIZE];
bool dataBlockUsed[MAX_DATA_BLOCKS];

typedef struct File {
    char name[100]; // 파일 이름
    int inodeNumber; // 파일의 inode 번호
    //char content[256]; // 파일 내용
    //Inode inode; // 파일의 inode 정보
} File;

typedef struct Directory {
    char name[100]; // 디렉터리 이름
    Inode inode; // 디렉터리의 inode 정보
    File files[100]; // 디렉터리 내의 파일 정보 배열
    int fileCount; // 현재 파일 개수
} Directory;

int allocateInode() {
    for (int i = 0; i < MAX_INODES; i++) {
        if (!inodeTable[i].inUse) {
            inodeTable[i].inUse = true;
            return i;
        }
    }
    return -1; // 사용 가능한 inode가 없는 경우
}

void freeInode(int inodeNumber) {
    inodeTable[inodeNumber].inUse = false;
}

int allocateDataBlock() {
    for (int i = 0; i < MAX_DATA_BLOCKS; i++) {
        if (!dataBlockUsed[i]) {
            dataBlockUsed[i] = true;
            return i;
        }
    }
    return -1;
}

void freeDataBlock(int blockNumber) {
    dataBlockUsed[blockNumber] = false;
}

void addFileToDirectory(Directory* dir, const char* fileName, int inodeNumber) {
    dir->files[dir->fileCount].inodeNumber = inodeNumber;
    strcpy(dir->files[dir->fileCount].name, fileName);
    dir->fileCount++;
}

void removeFileFromDirectory(Directory* dir, const char* fileName) {
    for (int i = 0; i < dir->fileCount; i++) {
        if (strcmp(dir->files[i].name, fileName) == 0) {
            for (int j = i; j < dir->fileCount - 1; j++) {
                dir->files[j] = dir->files[j + 1];
            }
            dir->fileCount--;
            break;
        }
    }
}

typedef enum { DIRECTORY, FILE_TYPE } NodeType;

typedef struct Node {
    NodeType type; // 노드 타입 (디렉터리 또는 파일)
    union {
        Directory dir;
        File file;
    };
    struct Node* parent; // 부모 노드 포인터
} Node;

Node* createNode(const char* name, NodeType type, Node* parent) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->type = type;
    newNode->parent = parent;
    if (type == DIRECTORY) {
        strcpy(newNode->dir.name, name);
        newNode->dir.childCount = 0;
        // inode 초기화
        newNode->dir.inode.fileSize = 0; // 디렉터리 크기는 자식 노드에 따라 달라질 수 있습니다.
        newNode->dir.inode.created = time(NULL);
        newNode->dir.inode.modified = time(NULL);
        newNode->dir.inode.linkCount = 0; // 초기 링크 수 설정
    } else {
        strcpy(newNode->file.name, name);
        memset(newNode->file.content, 0, sizeof(newNode->file.content)); // 파일 내용 초기화
        // inode 초기화
        newNode->file.inode.fileSize = strlen(newNode->file.content);
        newNode->file.inode.created = time(NULL);
        newNode->file.inode.modified = time(NULL);
        newNode->file.inode.linkCount = 1; // 파일의 경우 기본 링크 수는 1입니다.
    }
    return newNode;
}

void addChild(Node* parent, Node* child) {
    if (parent->dir.childCount < 10) {
        parent->dir.children[parent->dir.childCount++] = child;
    } else {
        printf("자식 노드의 최대 개수를 초과했습니다.\n");
    }
}

void printTree(Node* node, int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    if (node->type == DIRECTORY) {
        printf("%s/\n", node->dir.name);
    } else {
        printf("%s\n", node->file.name);
    }
    if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            printTree((Node*)node->dir.children[i], level + 1);
        }
    }
}

void freeTree(Node* node) {
    if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            freeTree((Node*)node->dir.children[i]);
        }
    }
    free(node);
}

Node* findNode(Node* node, const char* name, NodeType type) {
    if (node->type == type && strcmp(node->dir.name, name) == 0) {
        return node;
    }
    if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            Node* found = findNode((Node*)node->dir.children[i], name, type);
            if (found != NULL) {
                return found;
            }
        }
    }
    return NULL;
}

void updateFileContent(Node* fileNode, const char* newContent) {
    if (fileNode == NULL || fileNode->type != FILE_TYPE) {
        printf("유효하지 않은 파일 노드입니다.\n");
        return;
    }
    // 새로운 내용의 길이를 확인하여 파일의 내용을 업데이트
    int newContentSize = strlen(newContent);
    if (newContentSize >= sizeof(fileNode->file.content)) {
        printf("파일 내용이 너무 깁니다. 최대 길이는 %lu입니다.\n", sizeof(fileNode->file.content) - 1);
        return;
    }
    // 파일 내용 업데이트
    strcpy(fileNode->file.content, newContent);
    // 파일 크기 업데이트
    fileNode->file.inode.fileSize = newContentSize;
    // 파일 수정 시간 업데이트
    fileNode->file.inode.modified = time(NULL);
}

void readFile(int inodeNumber, char* buffer, int size) {
    Inode* inode = &inodeTable[inodeNumber];
    int bytesRead = 0;
    for (int i = 0; i < 10 && bytesRead < size; i++) {
        if (inode->dataBlockNumbers[i] != -1) {
            int bytesToCopy = BLOCK_SIZE;
            if (bytesRead + bytesToCopy > size) {
                bytesToCopy = size - bytesRead;
            }
            memcpy(buffer + bytesRead, dataBlocks[inode->dataBlockNumbers[i]], bytesToCopy);
            bytesRead += bytesToCopy;
        }
    }

    buffer[bytesRead] = '\0'; // 문자열 종료 문자 추가

    // 파일 내용 및 메타데이터 출력
    printf("파일 내용: %s\n", buffer);
    printf("파일 크기: %d bytes\n", inode->fileSize);

    // 생성 시간 출력
    char* createdTime = ctime(&inode->created);
    createdTime[strlen(createdTime) - 1] = '\0'; // 줄바꿈 제거
    printf("생성 시간: %s\n", createdTime);

    // 수정 시간 출력
    char* modifiedTime = ctime(&inode->modified);
    modifiedTime[strlen(modifiedTime) - 1] = '\0'; // 줄바꿈 제거
    printf("수정 시간: %s\n", modifiedTime);
}

void writeFile(int inodeNumber, const char* buffer, int size) {
    Inode* inode = &inodeTable[inodeNumber];
    int bytesWritten = 0;
    for (int i = 0; i < 10 && bytesWritten < size; i++) {
        if (inode->dataBlockNumbers[i] == -1) {
            int newBlockNumber = allocateDataBlock();
            inode->dataBlockNumbers[i] = newBlockNumber;
        }
        int bytesToCopy = BLOCK_SIZE;
        if (bytesWritten + bytesToCopy > size) {
            bytesToCopy = size - bytesWritten;
        }
        memcpy(dataBlocks[inode->dataBlockNumbers[i]], buffer + bytesWritten, bytesToCopy);
        bytesWritten += bytesToCopy;
    }
    inode->fileSize = size;
    inode->modified = time(NULL);
}

void updatefile(Node* parent, const char* name, const char* newContent) {
    if (parent->type != DIRECTORY) {
        printf("'%s'는 디렉터리가 아닙니다.\n", parent->dir.name);
        return;
    }

    bool found = false;
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == FILE_TYPE && strcmp(child->file.name, name) == 0) {
            strcpy(child->file.content, newContent);
            //child->file.inode.modified = time(NULL); // 파일 수정 시간 업데이트
            updateFileContent(child, newContent);
            printf("파일 '%s'의 내용이 업데이트 되었습니다.\n", name);
            found = true;
            break;
        }
    }
    if (!found) {
        printf("'%s' 파일을 찾을 수 없습니다.\n", name);
    }
}

void searchfile(Node* node, const char* keyword) {
    if (node->type == FILE_TYPE) {
        if (strstr(node->file.content, keyword) != NULL) {
            printf("키워드 '%s'를 포함하는 파일: %s\n", keyword, node->file.name);
            printf("해당 파일의 부모 디렉토리: %s\n", node->parent->dir.name);
            printf("파일 크기: %d바이트\n", node->file.inode.fileSize);
            
            // 생성 시간 출력
            char* createdTime = ctime(&node->file.inode.created);
            createdTime[strlen(createdTime) - 1] = '\0'; // 줄바꿈 제거
            printf("생성 시간: %s\n", createdTime);
            
            // 수정 시간 출력
            char* modifiedTime = ctime(&node->file.inode.modified);
            modifiedTime[strlen(modifiedTime) - 1] = '\0'; // 줄바꿈 제거
            printf("수정 시간: %s\n", modifiedTime);
        }
    } else if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            searchfile((Node*)node->dir.children[i], keyword);
        }
    }
}

int hasChildWithName(Node* parent, const char* name, int type) {
    if (parent->type != DIRECTORY) {
        return 0; // 부모가 디렉터리가 아니면 항상 0을 반환
    }
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (type == DIRECTORY && child->type == DIRECTORY && strcmp(child->dir.name, name) == 0) {
            return 1; // 동일한 이름의 디렉터리 발견
        } else if (type == FILE_TYPE && child->type == FILE_TYPE && strcmp(child->file.name, name) == 0) {
            return 1; // 동일한 이름의 파일 발견
        }
    }
    return 0; // 동일한 이름의 자식 노드 없음
}

void renameNode(Node* parent, const char* oldName, const char* newName, NodeType type) {
    if (parent->type != DIRECTORY) {
        printf("'%s'는 디렉터리가 아닙니다.\n", parent->dir.name);
        return;
    }
    // 같은 이름을 가진 자식 노드가 있는지 확인
    if (hasChildWithName(parent, newName, type)) {
        printf("'%s' 이름을 가진 %s가 이미 존재합니다.\n", newName, type == DIRECTORY ? "디렉터리" : "파일");
        return;
    }

    bool found = false;
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == type && strcmp(type == DIRECTORY ? child->dir.name : child->file.name, oldName) == 0) {
            // 이름 변경
            strcpy(type == DIRECTORY ? child->dir.name : child->file.name, newName);
            if (type == DIRECTORY) {
                child->dir.inode.modified = time(NULL); // 노드 수정 시간 업데이트
            } else {
                child->file.inode.modified = time(NULL); // 노드 수정 시간 업데이트
            }
            printf("'%s'의 이름이 '%s'(으)로 변경되었습니다.\n", oldName, newName);
            found = true;
            break;
        }
    }
    if (!found) {
        printf("'%s'를 찾을 수 없습니다.\n", oldName);
    }
}

void deleteNode(Node* parent, const char* name, NodeType type) {
    if (parent->type != DIRECTORY) {
        printf("'%s'는 디렉터리가 아닙니다.\n", parent->dir.name);
        return;
    }

    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == type && strcmp(type == DIRECTORY ? child->dir.name : child->file.name, name) == 0) {
            // 자식 노드 삭제 처리
            freeTree(child);
            // 배열에서 삭제된 노드 제거
            for (int j = i; j < parent->dir.childCount - 1; j++) {
                parent->dir.children[j] = parent->dir.children[j + 1];
            }
            parent->dir.childCount--;
            printf("'%s' %s가 삭제되었습니다.\n", name, type == DIRECTORY ? "디렉터리" : "파일");
            return;
        }
    }

    printf("'%s' %s를 찾을 수 없습니다.\n", name, type == DIRECTORY ? "디렉터리" : "파일");
}

void deepCopyNode(Node* original, Node* copy) {
    if (original->type == FILE_TYPE) {
        strcpy(copy->file.content, original->file.content);
        copy->file.inode.fileSize = original->file.inode.fileSize;
        copy->file.inode.created = time(NULL); // 복사 시점을 생성 시간으로 설정
        copy->file.inode.modified = time(NULL); // 복사 시점을 수정 시간으로 설정
        copy->file.inode.linkCount = 1; // 새 파일이므로 링크 수는 1
    } else if (original->type == DIRECTORY) {
        for (int i = 0; i < original->dir.childCount; i++) {
            Node* child = (Node*)original->dir.children[i];
            Node* newChild = createNode(child->type == DIRECTORY ? child->dir.name : child->file.name, child->type, copy);
            if (child->type == DIRECTORY) {
                copy->dir.inode.fileSize = 0; // 자식 노드에 따라 달라질 수 있으므로, 0으로 초기화
                copy->dir.inode.created = time(NULL); // 복사 시점을 생성 시간으로 설정
                copy->dir.inode.modified = time(NULL); // 복사 시점을 수정 시간으로 설정
                copy->dir.inode.linkCount = original->dir.inode.linkCount; // 링크 수는 원본 디렉터리의 링크 수와 동일하게 설정
                deepCopyNode(child, newChild);
            } else { // FILE_TYPE
                strcpy(newChild->file.content, child->file.content);
                copy->file.inode.fileSize = original->file.inode.fileSize;
                copy->file.inode.created = time(NULL); // 복사 시점을 생성 시간으로 설정
                copy->file.inode.modified = time(NULL); // 복사 시점을 수정 시간으로 설정
                copy->file.inode.linkCount = 1; // 새 파일이므로 링크 수는 1
            }
            addChild(copy, newChild);
        }
    }
}

void copyNode(Node* parent, const char* name, const char* newName, NodeType targetType, Node* targetParent) {
    if (parent->type != DIRECTORY) {
        printf("'%s'는 디렉터리가 아닙니다.\n", parent->dir.name);
        return;
    }
    if (targetParent->type != DIRECTORY) {
        printf("'%s'는 디렉터리가 아닙니다.\n", targetParent->dir.name);
        return;
    }
    if (hasChildWithName(targetParent, newName, targetType)) {
        printf("'%s' 이름을 가진 노드가 이미 '%s' 디렉터리에 존재합니다.\n", newName, targetParent->dir.name);
        return;
    }
    
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if ((child->type == targetType) && ((targetType == DIRECTORY && strcmp(child->dir.name, name) == 0) || (targetType == FILE_TYPE && strcmp(child->file.name, name) == 0))) {
            Node* newCopy = createNode(newName, child->type, targetParent);
            deepCopyNode(child, newCopy);
            addChild(targetParent, newCopy);
            printf("'%s'가 '%s'(으)로 복사되었습니다.\n", name, newName);
            return;
        }
    }
    printf("'%s'를 찾을 수 없습니다.\n", name);
}

void calculateDirectorySize(Node* node, int* totalSize) {
    if (node->type == FILE_TYPE) {
        *totalSize += node->file.inode.fileSize;
    } else if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            calculateDirectorySize((Node*)node->dir.children[i], totalSize);
        }
    }
}

void printDirectorySize(Node* node) {
    if (node == NULL || node->type != DIRECTORY) {
        printf("유효하지 않은 디렉터리 노드입니다.\n");
        return;
    }
    int totalSize = 0;
    calculateDirectorySize(node, &totalSize);
    printf("디렉터리 '%s'의 총 크기: %d bytes\n", node->dir.name, totalSize);
}


void dir_main() {
    Node* root = createNode("root", DIRECTORY, NULL);
    char command[100], name[100], parentName[100], content[256];

    while (1) {
        printf("명령을 입력하세요 (makedir, makefile, readfile, updatefile, searchfile, print, delete, rename, copy, dirsize, quit): ");
        scanf("%s", command);

        if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "makedir") == 0 || strcmp(command, "makefile") == 0) {
            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);

            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }

            printf("이름: ");
            scanf("%s", name);

            // 여기에 추가된 부분: 같은 이름의 자식 노드가 있는지 검사
            if (strcmp(command, "makedir") == 0) {
                int foundDirectory = hasChildWithName(parentNode, name, DIRECTORY);
                if (foundDirectory) {
                    printf("같은 이름의 디렉터리가 이미 존재합니다: %s\n", name);
                    continue; // 같은 이름의 노드가 있으면 명령을 건너뛴다
                }
            } else if (strcmp(command, "makefile") == 0) {
                int foundFile = hasChildWithName(parentNode, name, FILE_TYPE);
                if (foundFile) {
                    printf("같은 이름의 파일이 이미 존재합니다: %s\n", name);
                    continue; // 같은 이름의 노드가 있으면 명령을 건너뛴다
                }
            }

            if (strcmp(command, "makedir") == 0) {
                Node* newDir = createNode(name, DIRECTORY, parentNode);
                addChild(parentNode, newDir);
                printf("디렉터리 '%s'가 생성되었습니다.\n", name);
            } else { // makefile
                printf("파일 내용: ");
                scanf(" %[^\n]s", content); // 공백을 포함한 내용을 받기 위해 수정
                Node* newFile = createNode(name, FILE_TYPE, parentNode);
                strcpy(newFile->file.content, content);
                addChild(parentNode, newFile);
                updateFileContent(newFile, content);
                printf("파일 '%s'가 생성되었습니다.\n", name);
            }
        } else if (strcmp(command, "readfile") == 0) {
            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }
            printf("파일 이름: ");
            scanf("%s", name);
            readfile(parentNode, name);
        } else if (strcmp(command, "updatefile") == 0) {
            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }
            printf("파일 이름: ");
            scanf("%s", name);
            printf("새로운 파일 내용: ");
            scanf(" %[^\n]s", content); // 공백을 포함한 내용을 받기 위해 수정
            updatefile(parentNode, name, content);
        } else if (strcmp(command, "searchfile") == 0) {
            printf("검색할 키워드: ");
            scanf(" %[^\n]s", content); // 공백을 포함한 키워드를 받기 위해 수정
            searchfile(root, content);
        } else if (strcmp(command, "print") == 0) {
            printTree(root, 0);
        } else if (strcmp(command, "rename") ==0) {
            char oldName[100];
            char newName[100];
            NodeType type;
            char typeName[10];

            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }

            printf("변경할 파일/디렉터리의 이름: ");
            scanf("%s", oldName);
            printf("새 이름: ");
            scanf("%s", newName);
            printf("타입 ('file' 또는 'dir'): ");
            scanf("%s", typeName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;

            renameNode(parentNode, oldName, newName, type);
        } else if (strcmp(command, "delete") == 0) {
            NodeType type;
            char typeName[10];

            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }

            printf("삭제할 파일/디렉터리의 이름: ");
            scanf("%s", name);
            printf("타입 ('file' 또는 'dir'): ");
            scanf("%s", typeName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;

            deleteNode(parentNode, name, type);
        } else if (strcmp(command, "copy") == 0) {            
            char newName[100];
            char nodeName[100];
            NodeType type;
            char typeName[10];

            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }

            printf("복사할 파일/디렉터리의 이름: ");
            scanf("%s", name);
            printf("타입 ('file' 또는 'dir'): ");
            scanf("%s", typeName);
            printf("어디에 복사할지. 디렉터리의 이름: ");
            scanf("%s", nodeName);
            printf("복사할 파일/디렉터리의 새 이름: ");
            scanf("%s", newName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;
            
            Node* newNode = findNode(root, nodeName, DIRECTORY);

            copyNode(parentNode, name, newName, type, newNode);

        } else if(strcmp(command, "dirsize") == 0) {
            printf("부모 디렉터리 이름: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' 디렉터리를 찾을 수 없습니다.\n", parentName);
                continue;
            }
            printDirectorySize(parentNode);
        }
        else {
            printf("알 수 없는 명령입니다.\n");
        }
    }

    printTree(root, 0);
    freeTree(root);

}
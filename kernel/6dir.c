#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h> // ���� �ð� ������ ���� �߰�

typedef struct Inode {
    int fileSize; // ���� ũ��
    time_t created; // ���� ���� �ð�
    time_t modified; // ���� ���� �ð�
    int linkCount; // ��ũ ��
    // ���⿡ �� ���� inode ���� ������ �߰��� �� �ֽ��ϴ�.
} Inode;

typedef struct File {
    char name[100]; // ���� �̸�
    char content[256]; // ���� ����
    Inode inode; // ������ inode ����
} File;

typedef struct Directory {
    char name[100]; // ���͸� �̸�
    void* children[10]; // �ڽ� ��� ������ �迭 (���͸� �Ǵ� ����), ������ ������ ���� �ִ� 10���� ����
    int childCount; // ���� �ڽ� ����� ��
    Inode inode; // ���͸��� inode ����
} Directory;

typedef enum { DIRECTORY, FILE_TYPE } NodeType;

typedef struct Node {
    NodeType type; // ��� Ÿ�� (���͸� �Ǵ� ����)
    union {
        Directory dir;
        File file;
    };
    struct Node* parent; // �θ� ��� ������
} Node;

Node* createNode(const char* name, NodeType type, Node* parent) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->type = type;
    newNode->parent = parent;
    if (type == DIRECTORY) {
        strcpy(newNode->dir.name, name);
        newNode->dir.childCount = 0;
        // inode �ʱ�ȭ
        newNode->dir.inode.fileSize = 0; // ���͸� ũ��� �ڽ� ��忡 ���� �޶��� �� �ֽ��ϴ�.
        newNode->dir.inode.created = time(NULL);
        newNode->dir.inode.modified = time(NULL);
        newNode->dir.inode.linkCount = 0; // �ʱ� ��ũ �� ����
    } else {
        strcpy(newNode->file.name, name);
        memset(newNode->file.content, 0, sizeof(newNode->file.content)); // ���� ���� �ʱ�ȭ
        // inode �ʱ�ȭ
        newNode->file.inode.fileSize = strlen(newNode->file.content);
        newNode->file.inode.created = time(NULL);
        newNode->file.inode.modified = time(NULL);
        newNode->file.inode.linkCount = 1; // ������ ��� �⺻ ��ũ ���� 1�Դϴ�.
    }
    return newNode;
}

void addChild(Node* parent, Node* child) {
    if (parent->dir.childCount < 10) {
        parent->dir.children[parent->dir.childCount++] = child;
    } else {
        printf("�ڽ� ����� �ִ� ������ �ʰ��߽��ϴ�.\n");
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
        printf("��ȿ���� ���� ���� ����Դϴ�.\n");
        return;
    }
    // ���ο� ������ ���̸� Ȯ���Ͽ� ������ ������ ������Ʈ
    int newContentSize = strlen(newContent);
    if (newContentSize >= sizeof(fileNode->file.content)) {
        printf("���� ������ �ʹ� ��ϴ�. �ִ� ���̴� %lu�Դϴ�.\n", sizeof(fileNode->file.content) - 1);
        return;
    }
    // ���� ���� ������Ʈ
    strcpy(fileNode->file.content, newContent);
    // ���� ũ�� ������Ʈ
    fileNode->file.inode.fileSize = newContentSize;
    // ���� ���� �ð� ������Ʈ
    fileNode->file.inode.modified = time(NULL);
}

void readfile(Node* node, const char* name) {
    bool found = false;
    if (node->type == FILE_TYPE) {
        if (strcmp(node->file.name, name) == 0) {
            printf("������ �θ� ���丮: %s ", node->parent->dir.name);
            printf("���� ����: %s\n", node->file.content);
            printf("���� ũ��: %d bytes \n", node->file.inode.fileSize);

            // ���� �ð� ���
            char* createdTime = ctime(&node->file.inode.created);
            createdTime[strlen(createdTime) - 1] = '\0'; // �ٹٲ� ����
            printf("���� �ð�: %s\n", createdTime);
            
            // ���� �ð� ���
            char* modifiedTime = ctime(&node->file.inode.modified);
            modifiedTime[strlen(modifiedTime) - 1] = '\0'; // �ٹٲ� ����
            printf("���� �ð�: %s\n", modifiedTime);

            found = true;
        }
    } else if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            Node* child = (Node*)node->dir.children[i];
            if (child->type == FILE_TYPE && strcmp(child->file.name, name) == 0) {
                printf("������ �θ� ���丮: %s ", node->dir.name);
                printf("���� ����: %s\n", child->file.content);
                printf("���� ũ��: %d bytes \n", child->file.inode.fileSize);

                // ���� �ð� ���
                char* createdTime = ctime(&child->file.inode.created);
                createdTime[strlen(createdTime) - 1] = '\0'; // �ٹٲ� ����
                printf("���� �ð�: %s\n", createdTime);
            
                // ���� �ð� ���
                char* modifiedTime = ctime(&child->file.inode.modified);
                modifiedTime[strlen(modifiedTime) - 1] = '\0'; // �ٹٲ� ����
                printf("���� �ð�: %s\n", modifiedTime);

                found = true;
            } else if (child->type == DIRECTORY) {
                readfile(child, name); // ��������� Ž��
            }
        }
    }
    if (!found && node->parent == NULL) { // �ֻ��� ��忡�� ������ ã�� ��������
        printf("'%s' ������ ã�� �� �����ϴ�.\n", name);
    }
}


void updatefile(Node* parent, const char* name, const char* newContent) {
    if (parent->type != DIRECTORY) {
        printf("'%s'�� ���͸��� �ƴմϴ�.\n", parent->dir.name);
        return;
    }

    bool found = false;
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == FILE_TYPE && strcmp(child->file.name, name) == 0) {
            strcpy(child->file.content, newContent);
            //child->file.inode.modified = time(NULL); // ���� ���� �ð� ������Ʈ
            updateFileContent(child, newContent);
            printf("���� '%s'�� ������ ������Ʈ �Ǿ����ϴ�.\n", name);
            found = true;
            break;
        }
    }
    if (!found) {
        printf("'%s' ������ ã�� �� �����ϴ�.\n", name);
    }
}

void searchfile(Node* node, const char* keyword) {
    if (node->type == FILE_TYPE) {
        if (strstr(node->file.content, keyword) != NULL) {
            printf("Ű���� '%s'�� �����ϴ� ����: %s\n", keyword, node->file.name);
            printf("�ش� ������ �θ� ���丮: %s\n", node->parent->dir.name);
            printf("���� ũ��: %d����Ʈ\n", node->file.inode.fileSize);
            
            // ���� �ð� ���
            char* createdTime = ctime(&node->file.inode.created);
            createdTime[strlen(createdTime) - 1] = '\0'; // �ٹٲ� ����
            printf("���� �ð�: %s\n", createdTime);
            
            // ���� �ð� ���
            char* modifiedTime = ctime(&node->file.inode.modified);
            modifiedTime[strlen(modifiedTime) - 1] = '\0'; // �ٹٲ� ����
            printf("���� �ð�: %s\n", modifiedTime);
        }
    } else if (node->type == DIRECTORY) {
        for (int i = 0; i < node->dir.childCount; i++) {
            searchfile((Node*)node->dir.children[i], keyword);
        }
    }
}

int hasChildWithName(Node* parent, const char* name, int type) {
    if (parent->type != DIRECTORY) {
        return 0; // �θ� ���͸��� �ƴϸ� �׻� 0�� ��ȯ
    }
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (type == DIRECTORY && child->type == DIRECTORY && strcmp(child->dir.name, name) == 0) {
            return 1; // ������ �̸��� ���͸� �߰�
        } else if (type == FILE_TYPE && child->type == FILE_TYPE && strcmp(child->file.name, name) == 0) {
            return 1; // ������ �̸��� ���� �߰�
        }
    }
    return 0; // ������ �̸��� �ڽ� ��� ����
}

void renameNode(Node* parent, const char* oldName, const char* newName, NodeType type) {
    if (parent->type != DIRECTORY) {
        printf("'%s'�� ���͸��� �ƴմϴ�.\n", parent->dir.name);
        return;
    }
    // ���� �̸��� ���� �ڽ� ��尡 �ִ��� Ȯ��
    if (hasChildWithName(parent, newName, type)) {
        printf("'%s' �̸��� ���� %s�� �̹� �����մϴ�.\n", newName, type == DIRECTORY ? "���͸�" : "����");
        return;
    }

    bool found = false;
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == type && strcmp(type == DIRECTORY ? child->dir.name : child->file.name, oldName) == 0) {
            // �̸� ����
            strcpy(type == DIRECTORY ? child->dir.name : child->file.name, newName);
            if (type == DIRECTORY) {
                child->dir.inode.modified = time(NULL); // ��� ���� �ð� ������Ʈ
            } else {
                child->file.inode.modified = time(NULL); // ��� ���� �ð� ������Ʈ
            }
            printf("'%s'�� �̸��� '%s'(��)�� ����Ǿ����ϴ�.\n", oldName, newName);
            found = true;
            break;
        }
    }
    if (!found) {
        printf("'%s'�� ã�� �� �����ϴ�.\n", oldName);
    }
}

void deleteNode(Node* parent, const char* name, NodeType type) {
    if (parent->type != DIRECTORY) {
        printf("'%s'�� ���͸��� �ƴմϴ�.\n", parent->dir.name);
        return;
    }

    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if (child->type == type && strcmp(type == DIRECTORY ? child->dir.name : child->file.name, name) == 0) {
            // �ڽ� ��� ���� ó��
            freeTree(child);
            // �迭���� ������ ��� ����
            for (int j = i; j < parent->dir.childCount - 1; j++) {
                parent->dir.children[j] = parent->dir.children[j + 1];
            }
            parent->dir.childCount--;
            printf("'%s' %s�� �����Ǿ����ϴ�.\n", name, type == DIRECTORY ? "���͸�" : "����");
            return;
        }
    }

    printf("'%s' %s�� ã�� �� �����ϴ�.\n", name, type == DIRECTORY ? "���͸�" : "����");
}

void deepCopyNode(Node* original, Node* copy) {
    if (original->type == FILE_TYPE) {
        strcpy(copy->file.content, original->file.content);
        copy->file.inode.fileSize = original->file.inode.fileSize;
        copy->file.inode.created = time(NULL); // ���� ������ ���� �ð����� ����
        copy->file.inode.modified = time(NULL); // ���� ������ ���� �ð����� ����
        copy->file.inode.linkCount = 1; // �� �����̹Ƿ� ��ũ ���� 1
    } else if (original->type == DIRECTORY) {
        for (int i = 0; i < original->dir.childCount; i++) {
            Node* child = (Node*)original->dir.children[i];
            Node* newChild = createNode(child->type == DIRECTORY ? child->dir.name : child->file.name, child->type, copy);
            if (child->type == DIRECTORY) {
                copy->dir.inode.fileSize = 0; // �ڽ� ��忡 ���� �޶��� �� �����Ƿ�, 0���� �ʱ�ȭ
                copy->dir.inode.created = time(NULL); // ���� ������ ���� �ð����� ����
                copy->dir.inode.modified = time(NULL); // ���� ������ ���� �ð����� ����
                copy->dir.inode.linkCount = original->dir.inode.linkCount; // ��ũ ���� ���� ���͸��� ��ũ ���� �����ϰ� ����
                deepCopyNode(child, newChild);
            } else { // FILE_TYPE
                strcpy(newChild->file.content, child->file.content);
                copy->file.inode.fileSize = original->file.inode.fileSize;
                copy->file.inode.created = time(NULL); // ���� ������ ���� �ð����� ����
                copy->file.inode.modified = time(NULL); // ���� ������ ���� �ð����� ����
                copy->file.inode.linkCount = 1; // �� �����̹Ƿ� ��ũ ���� 1
            }
            addChild(copy, newChild);
        }
    }
}

void copyNode(Node* parent, const char* name, const char* newName, NodeType targetType, Node* targetParent) {
    if (parent->type != DIRECTORY) {
        printf("'%s'�� ���͸��� �ƴմϴ�.\n", parent->dir.name);
        return;
    }
    if (targetParent->type != DIRECTORY) {
        printf("'%s'�� ���͸��� �ƴմϴ�.\n", targetParent->dir.name);
        return;
    }
    if (hasChildWithName(targetParent, newName, targetType)) {
        printf("'%s' �̸��� ���� ��尡 �̹� '%s' ���͸��� �����մϴ�.\n", newName, targetParent->dir.name);
        return;
    }
    
    for (int i = 0; i < parent->dir.childCount; i++) {
        Node* child = (Node*)parent->dir.children[i];
        if ((child->type == targetType) && ((targetType == DIRECTORY && strcmp(child->dir.name, name) == 0) || (targetType == FILE_TYPE && strcmp(child->file.name, name) == 0))) {
            Node* newCopy = createNode(newName, child->type, targetParent);
            deepCopyNode(child, newCopy);
            addChild(targetParent, newCopy);
            printf("'%s'�� '%s'(��)�� ����Ǿ����ϴ�.\n", name, newName);
            return;
        }
    }
    printf("'%s'�� ã�� �� �����ϴ�.\n", name);
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
        printf("��ȿ���� ���� ���͸� ����Դϴ�.\n");
        return;
    }
    int totalSize = 0;
    calculateDirectorySize(node, &totalSize);
    printf("���͸� '%s'�� �� ũ��: %d bytes\n", node->dir.name, totalSize);
}


int main() {
    Node* root = createNode("root", DIRECTORY, NULL);
    char command[100], name[100], parentName[100], content[256];

    while (1) {
        printf("������ �Է��ϼ��� (makedir, makefile, readfile, updatefile, searchfile, print, delete, rename, copy, dirsize, quit): ");
        scanf("%s", command);

        if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "makedir") == 0 || strcmp(command, "makefile") == 0) {
            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);

            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }

            printf("�̸�: ");
            scanf("%s", name);

            // ���⿡ �߰��� �κ�: ���� �̸��� �ڽ� ��尡 �ִ��� �˻�
            if (strcmp(command, "makedir") == 0) {
                int foundDirectory = hasChildWithName(parentNode, name, DIRECTORY);
                if (foundDirectory) {
                    printf("���� �̸��� ���͸��� �̹� �����մϴ�: %s\n", name);
                    continue; // ���� �̸��� ��尡 ������ ������ �ǳʶڴ�
                }
            } else if (strcmp(command, "makefile") == 0) {
                int foundFile = hasChildWithName(parentNode, name, FILE_TYPE);
                if (foundFile) {
                    printf("���� �̸��� ������ �̹� �����մϴ�: %s\n", name);
                    continue; // ���� �̸��� ��尡 ������ ������ �ǳʶڴ�
                }
            }

            if (strcmp(command, "makedir") == 0) {
                Node* newDir = createNode(name, DIRECTORY, parentNode);
                addChild(parentNode, newDir);
                printf("���͸� '%s'�� �����Ǿ����ϴ�.\n", name);
            } else { // makefile
                printf("���� ����: ");
                scanf(" %[^\n]s", content); // ������ ������ ������ �ޱ� ���� ����
                Node* newFile = createNode(name, FILE_TYPE, parentNode);
                strcpy(newFile->file.content, content);
                addChild(parentNode, newFile);
                updateFileContent(newFile, content);
                printf("���� '%s'�� �����Ǿ����ϴ�.\n", name);
            }
        } else if (strcmp(command, "readfile") == 0) {
            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }
            printf("���� �̸�: ");
            scanf("%s", name);
            readfile(parentNode, name);
        } else if (strcmp(command, "updatefile") == 0) {
            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }
            printf("���� �̸�: ");
            scanf("%s", name);
            printf("���ο� ���� ����: ");
            scanf(" %[^\n]s", content); // ������ ������ ������ �ޱ� ���� ����
            updatefile(parentNode, name, content);
        } else if (strcmp(command, "searchfile") == 0) {
            printf("�˻��� Ű����: ");
            scanf(" %[^\n]s", content); // ������ ������ Ű���带 �ޱ� ���� ����
            searchfile(root, content);
        } else if (strcmp(command, "print") == 0) {
            printTree(root, 0);
        } else if (strcmp(command, "rename") ==0) {
            char oldName[100];
            char newName[100];
            NodeType type;
            char typeName[10];

            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }

            printf("������ ����/���͸��� �̸�: ");
            scanf("%s", oldName);
            printf("�� �̸�: ");
            scanf("%s", newName);
            printf("Ÿ�� ('file' �Ǵ� 'dir'): ");
            scanf("%s", typeName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;

            renameNode(parentNode, oldName, newName, type);
        } else if (strcmp(command, "delete") == 0) {
            NodeType type;
            char typeName[10];

            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }

            printf("������ ����/���͸��� �̸�: ");
            scanf("%s", name);
            printf("Ÿ�� ('file' �Ǵ� 'dir'): ");
            scanf("%s", typeName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;

            deleteNode(parentNode, name, type);
        } else if (strcmp(command, "copy") == 0) {            
            char newName[100];
            char nodeName[100];
            NodeType type;
            char typeName[10];

            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }

            printf("������ ����/���͸��� �̸�: ");
            scanf("%s", name);
            printf("Ÿ�� ('file' �Ǵ� 'dir'): ");
            scanf("%s", typeName);
            printf("��� ��������. ���͸��� �̸�: ");
            scanf("%s", nodeName);
            printf("������ ����/���͸��� �� �̸�: ");
            scanf("%s", newName);
            type = strcmp(typeName, "dir") == 0 ? DIRECTORY : FILE_TYPE;
            
            Node* newNode = findNode(root, nodeName, DIRECTORY);

            copyNode(parentNode, name, newName, type, newNode);

        } else if(strcmp(command, "dirsize") == 0) {
            printf("�θ� ���͸� �̸�: ");
            scanf("%s", parentName);
            Node* parentNode = findNode(root, parentName, DIRECTORY);
            if (parentNode == NULL || parentNode->type != DIRECTORY) {
                printf("'%s' ���͸��� ã�� �� �����ϴ�.\n", parentName);
                continue;
            }
            printDirectorySize(parentNode);
        }
        else {
            printf("�� �� ���� �����Դϴ�.\n");
        }
    }

    printTree(root, 0);
    freeTree(root);

    return 0;
}
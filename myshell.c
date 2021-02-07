#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <assert.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_BG_PROCESS 15

#define READ_FLAGS (O_RDONLY) // Open for reading only
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC) // Open for write only, create file if it does not exist, truncate size to 0
#define APPEND_FLAGS (O_WRONLY | O_CREAT | O_APPEND) // Open for write only, create file if it does not exist, append on each write
#define READ_PERMISSION (S_IRUSR | S_IRGRP | S_IROTH) // Read permissions (owner,group,others)
#define CREATE_PERMISSION (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) // Read permission for owner, write permission, read permission for group and other

typedef struct{ // For background processes
    int processpid[MAX_BG_PROCESS];
    char processName[MAX_BG_PROCESS][50];
    int bgProcessCount;
}bg;

bg bgProcesses;

int foregroundPid; // foreground process id.It may change by executing a new process
char fgProcessName[50]; // process name related to corresponding process

pid_t mainProcessPID; // main process pid
int numOfArgs; // number of arguments entered

pid_t childpid; //childpid
int counter = 0; // counts number of processes in bookmark list
int background; /* equals 1 if a command is followed by '&' */
int backgroundChildNum = 0;

int pathLength; // length of the path variable
char *paths[MAX_LINE];

int inputFlag; // flag for input redirection
int outputFlag; // flag for output redirection
 
char outputSymbol[2] = {"00"}; // storing output symbol entered
char inputName[20];
char outputName[20];

void setup(char inputBuffer[], char *args[], int *background);
void parsePath();
void signalToStopHandler();
void ioRedirection(char *args[]);
void backgroundProcesses();
int search(char *args[], int *background);
void removeChars(char *str, char c);
int searchInFile (char *fname, char *str);
void recursiveSearch(char *path, char *str);
bool checkValidInt(const char *str);
char **splitStr(char *a_str, const char a_delim);
void bookmarkHandleI(char *args[]);
struct list *searchList(int val, struct list **prev);
int deleteFromBookmark(int val);
struct list *addToList(int val, char commandStr[128], bool add_to_end);
struct list *createList(int val, char commandStr[128]);
void bookmarkExecute(char *args[], int *background);
int commandArgs(char *args[]);
void parentSection(char *args[], int background, pid_t childpid);
void createNewProcess(char *args[], int background);
void freePath();
void childSection(char *args[]);
char *splitText(char *text, char regex, int index);

struct list {
    int val; // index number
    char commandName[128]; // arguments to bookmarkExecute
    struct list *next; // next pointer
};

struct list *head = NULL; // initially there are no bookmarks 
struct list *current = NULL;  // current bookmark pointer that will point to

 // strtok function in string.h library
char *splitText(char *text, char regex, int index){
    char *indexPointer = text;
    int arrayIndex = 0;
    int indexRepetition = 0;
    char *returnValue = malloc(MAX_LINE * sizeof(char));
    while(*indexPointer){
        if(*indexPointer == regex && indexRepetition == index)
            break;
        else if(*indexPointer == regex && indexRepetition < index)
            indexRepetition++;

        if(indexRepetition == index && *indexPointer != regex)
            returnValue[arrayIndex++] = *indexPointer;

        indexPointer++;
    }
    if(returnValue[0] == '\0')
        returnValue = NULL;

    return returnValue;
}

void freePath(){
    for(int i = 0; i < pathLength; i++){
        free(paths[i]);
    }
}

// handling of I/O redirections and exec command
void childSection(char *args[]){
    //We need to bookmarkExecute execl for every path.
    char path[MAX_LINE];
    int inputDir; // input file directory
    int outputDir; // output file directory

    if(inputFlag == 1){ // If input redirection is required
        inputDir = open(inputName, READ_FLAGS, READ_PERMISSION); // Used to Open the file for reading
        if(inputDir == -1){ // if input file can not be opened
            perror("File could not opened");
            return;
        }
        if(dup2(inputDir, STDIN_FILENO) == -1){ // redirection is failed
            perror("File could not redirected");
            return;
        }
        if(close(inputDir) == -1){ // if input file can not be closed
            perror("Error occured while closing file");
            return;
        }
    }

    if(outputFlag == 1){ // If output redirection is required
        
        int outputControl = -1 ; // 0 for > , 1 for >> , 2 for 2>, -1 for error
        if(strcmp(outputSymbol, ">") == 0) outputControl = 0;
        else if(strcmp(outputSymbol, ">>") == 0) outputControl = 1;
        else if(strcmp(outputSymbol, "2>") == 0) outputControl = 2;
        //Used to Open the file for writing or creating  
        if(outputControl == 0 || outputControl == 2) outputDir = open(outputName, CREATE_FLAGS, CREATE_PERMISSION); 
        else outputDir = open(outputName, APPEND_FLAGS, CREATE_PERMISSION); //Used to Open the file for writing, creating or appending  

        if(outputDir == -1){ // if file creation or append is failed
            perror("Creation or append is failed");
            return;
        }

        if(outputControl == -1 ){ // input is not >, >> or 2>
            perror("Output argument is not valid");
            return;
        } else if(outputControl == 0 || outputControl == 1){ // redirection is required
            if(dup2(outputDir, STDOUT_FILENO) == -1){ // check if redirection is completed successfully
                perror("Failed to redirect for output");
                return;
            }
        }else{
            if(dup2(outputDir, STDERR_FILENO) == -1){ // redirection is failed
                perror("Failed to redirect for output");
                return;
            }
        }

        if(close(outputDir) == -1){  // if output file can not be closed
            perror("Error occured while closing file");
            return;
        }
    }

    for(int i = 0; i < pathLength; i++){
        strcpy(path, paths[i]);
        strcat(path, "/"); //First we need to append '/';
        strcat(path, args[0]); // append file name to path(args[0] in this case)
        
        execl(path, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
                args[10], args[11], args[12], args[13], args[14], NULL);
        
    }
    perror("Child failed to bookmarkExecute execl command");
    exit(errno);
}

// this function creates new processes
void createNewProcess(char *args[], int background){
    pid_t childpid;
    childpid = fork();
    if(childpid == -1){ // if child is not created successfully
        perror("Child is not created successfully\n");
        return;
    }else if(childpid != 0){ // parent process
        parentSection(args, background, childpid);
    }else{ // child process
        childSection(args);
    }
}

// creating a new process for parent
void parentSection(char *args[], int background, pid_t childpid){
    if(background == 1){ // if & given after command
        if(bgProcesses.bgProcessCount == MAX_BG_PROCESS){ // Check if we reach to maximum background process number
            fprintf(stderr, "No space left in background!\n");

            /* WUNTRACED allows  parent to be returned from wait()/waitpid() 
            if a child gets stopped as well as exiting or being killed.  */
            waitpid(childpid, NULL, WUNTRACED);
        }else{ // available space exists to add a new background process           
            for(int i = 0; i < MAX_BG_PROCESS; i++){
                if(bgProcesses.processpid[i] == 0){
                    setpgid(childpid, childpid); // setting process group id
                    // Adding new process to background processes with id and name
                    bgProcesses.processpid[i] = childpid;
                    bgProcesses.bgProcessCount++;
                    strcpy(bgProcesses.processName[i], args[0]);                      
                    break;
                }
            }
        }
    }else{
        setpgid(childpid, childpid); // setting process group id
        foregroundPid = childpid;  // assigning current process id to foreground process' id
        strcpy(fgProcessName, args[0]);

        /* WUNTRACED allows  parent to be returned from wait()/waitpid() 
            if a child gets stopped as well as exiting or being killed.  */
        if(childpid != waitpid(childpid, NULL, WUNTRACED))
            perror("Parent failed while waiting the child due to a signal or error!!!");
    }
}

// Reads all commandArgs
int commandArgs(char *args[]){
    // returns 0 value will return if entered argument is a valid command
    // returns 1 value will return for exit conditions
    // returns 2 value will return if entered argument is not valid
    int caseNum = -1; 

    if(strcmp(args[0], "ps_all") == 0)
        caseNum = 0;
    else if(strcmp(args[0], "search") == 0)
        caseNum = 1;
    else if(strcmp(args[0], "bookmark") == 0)
        caseNum = 2;
    else if(strcmp(args[0], "exit") == 0)
        caseNum = 3;

    switch(caseNum){
        case 0: // ps_all    
            backgroundProcesses();
            return 0;
        case 1: // search
            search(args, &background);
            return 0;    
        case 2: // bookmark
            if (args[0] == NULL) return 0;

            if (!strcmp(args[0], "bookmark") && args[1] == NULL){ 
                fprintf(stderr, "Input is invalid\n");
                return 0;
            }

            if (!strcmp(args[0], "bookmark") && !strcmp(args[1], "-i")){ // if given argument is in 'bookmark -i' format
                if (args[2] == NULL || !checkValidInt(args[2])) { // entering number after 'bookmark -i' is mandatory
                    fprintf(stderr, "Invalid input.\n");
                    return 0;
                }
                bookmarkHandleI(args); // handle bookmark -i command 
            }
            else {
                bookmarkExecute(args, &background); // handle bookmark commandArgs
            }
            return 0;
        case 3: // exit
            // after execution with & process added to background processes
            // if backround processes are not terminated dont allow system to bookmarkExecute exit command    
            backgroundProcesses(); // check if there exists background processes before exiting
            if(bgProcesses.bgProcessCount > 0){
                fprintf(stderr, "%d processes are not terminated. Terminate them before exiting the shell.\n", bgProcesses.bgProcessCount);
                return 0;
            }else printf("There is no running background process\n"); // if there is no background process left
                
            return 1;              
    }
    return 2;
}

// This function will bookmarkExecute bookmark commandArgs
void bookmarkExecute(char *args[], int *background) {
    int index = 0;
    while (args[index] != NULL) index++; //Count how many arguments given

    if (!strcmp(args[0], "bookmark")) {
        if (!strcmp(args[0],"bookmark") && !strcmp(args[1], "-d")) { // bookmark deletion operation
            if (args[2] == NULL || !checkValidInt(args[2])){ //Check if input is null
                printf("Input is invalid. You should enter a proper number.\n");
                return;
            }
            int deleteIndex = atoi(args[2]); // convert string to int
            struct list *temp = head;

            int deleteFound = 0; // condition to check is index found or not

            while (temp != NULL) { // check whether if initially there are bookmarks in head pointer
                if (temp->val == deleteIndex) { // find the appropriate bookmark
                    deleteFromBookmark(deleteIndex); // if it is found, delete corresponding bookmark
                    counter--;  // decrement number of bookmarks from bookmark list
                    deleteFound = 1; // index is found 
                    break;
                } else {
                    temp = temp->next;
                }
            }
          
            if (deleteFound == 0) { // index not found
                fprintf(stderr, "Not found bookmark with %d\n", deleteIndex);
                return;
            }

            int structCtr = 0; // initialize index to flush it
            temp = head; // point to head since we should reinitialize pointer list
            while (temp != NULL) { // shift to left since one of the bookmarks is deleted
                temp->val = structCtr;
                structCtr++;
                temp = temp->next;
            }
            return;
        } else if (!strcmp(args[0], "bookmark") && !strcmp(args[1], "-l")) {// bookmark -l operation to list bookmarks
            struct list *temp = head;
            if (args[2] != NULL) { // not acceptable more arguments after -l command
                fprintf(stderr, "Input is invalid\n");
                return;
            }

            if (temp == NULL) fprintf(stderr, "There are no bookmarks to view.\n"); // if there are no bookmarks exists
          
            while (temp != NULL) {   //print existing bookmarks
                printf("%d      ", temp->val);
                printf("%s\n", temp->commandName);
                temp = temp->next;
            }
            return;
        } else { // Adding a new bookmark operation to bookmark list
            char bookmarkStr[128];
            strcpy(bookmarkStr, "");
            
            for (int i = 0; i < index - 1; i++) { // shift args left, by one 
                strcpy(args[i], args[i + 1]);
            }
            
            index = index - 1;
            for (int i = 0; i < index; i++) {
                strcat(bookmarkStr, args[i]);
                if (i != index - 1) { // "ls-l" should in a format like "ls -l"
                    strcat(bookmarkStr, " ");
                }
            }
            
            int structCtr = 0;
            struct list *temp = head;
            while (temp != NULL) { 
                temp->val = structCtr;
                structCtr++;
                temp = temp->next;
            }

            addToList(counter, bookmarkStr, 1); // Creating new bookmark with given arguments 
            counter++; // bookmark list counter

            structCtr = 0;
            temp = head;
            while (temp != NULL) { // reorganize link list for getting correct indexes
                temp->val = structCtr;
                structCtr++;
                temp = temp->next;
            }
            return;
        }
    }
    
    if (*background == 1) { // means command entered with &
        childpid = fork(); 
        backgroundChildNum++; // child number in background processes
        
        if (childpid == -1) { // fork is not completed successfully
            fprintf(stderr, "Child is not created successfully\n");
            return;
        }
        
        if (childpid == 0) { // child process is created successfully
            struct dirent *file; 
            char *pathArr = getenv("PATH"); // Get path
            pathArr = strtok(strdup(pathArr), ":"); // Tokenize ':' from path

            int index = 0;
            while (args[index] != NULL) index++; // Count number of arguments currently
        }

        if (childpid > 0) return;
        
    } else if (*background == 0) { // means command entered without &
        childpid = fork(); // fork the child
        backgroundChildNum++;

        if (childpid == -1) { // fork is not completed successfully
            fprintf(stderr, "Child is not created successfully\n");
            return;
        }
       
        if (childpid == 0) { // child process is created successfully
            struct dirent *file;
            char *pathArr = getenv("PATH"); // Get path
            pathArr = strtok(strdup(pathArr), ":"); // Tokenize ':' from path

            int index = 0;
            while (args[index] != NULL) index++; // Count number of arguments currently

            //While the path is not null.
            while (pathArr != NULL) {
                DIR *dir;
                //Open the path directory
                dir = opendir(pathArr);
               
                if (dir) {  // If directory opened correctly
                    while ((file = readdir(dir)) != NULL) { // if current file is readable
                        if (strcmp(file->d_name, args[0]) == 0) { // if file is current file to bookmarkExecute from args
                            strcat(pathArr, "/"); // Add  / to  path 
                            strcat(pathArr, args[0]);
                            if (execv(pathArr, args) == -1) {  
                                fprintf(stderr, "Child process couldn't run \n"); // if execution fails
                            }
                        }
                    }
                }
                pathArr = strtok(NULL, ":"); // make empty path
            }
            fprintf(stderr, "Command is not executable\n"); // if given command in bookmark is not found 
        }
        if (childpid > 0) { // parent process
            waitpid(childpid, NULL, 0); // It returns 0 to report that there are possibly unwaited-for children but that their status is not available
            return;
        }
    }
    else {
        return;
    }
}

// Creates a new list if initially list is null
struct list *createList(int val, char commandStr[128]) {
    struct list *ptr = (struct list *)malloc(sizeof(struct list));
    if (NULL == ptr){
        fprintf(stderr, "\n Node creation failed \n");
        return NULL;
    }
    ptr->val = val;
    strcpy(ptr->commandName, commandStr);
    ptr->next = NULL;
    head = current = ptr;
    return ptr;
}

// Function to add given bookmark into bookmark list
struct list *addToList(int val, char commandStr[128], bool add_to_end) {
    if (NULL == head) { // if initially list is empty, create it.
        return (createList(val, commandStr));
    }
    struct list *ptr = (struct list *)malloc(sizeof(struct list));
    if (NULL == ptr) {
        fprintf(stderr, "\n Node creation failed \n");
        return NULL;
    }
    ptr->val = val;
    strcpy(ptr->commandName, commandStr);
    ptr->next = NULL;

    if (add_to_end){
        current->next = ptr;
        current = ptr;
    }
    else{
        ptr->next = head;
        head = ptr;
    }
    return ptr;
}

// Bookmark -d operation 
int deleteFromBookmark(int val) {
    struct list *prev = NULL;
    struct list *del = NULL;
    del = searchList(val, &prev);
    if (del == NULL) {
        return -1;
    } else {
        if (prev != NULL) {
            prev->next = del->next;
        }
        if (del == head){
            head = del->next;
        }
        else if (del == current){
            current = prev;
        }
    }
    free(del);
    del = NULL;
    return 0;
}

// search given index value from list of pointers for bookmark delete operation
struct list *searchList(int val, struct list **prev) {
    struct list *ptr = head;
    struct list *tmp = NULL;
    bool found = false;
    while (ptr != NULL) {
        if (ptr->val == val) {
            found = true;
            break;
        } else {
            tmp = ptr;
            ptr = ptr->next;
        }
    }
    if (true == found){
        if (prev)
        *prev = tmp;
        return ptr;
    }
    else{
        return NULL;
    }
}

// bookmark with -i argument
void bookmarkHandleI(char *args[]){
    struct list *temp = head;
    int indexOfNum = atoi(args[2]); // getting last arg as number
    int isFound = 0;
    char *strToExecute;

    while (temp != NULL){ // currently check if temp pointer is not null
        if (temp->val == indexOfNum) { //search for the index if it is found remove its ""
            strToExecute = strdup(temp->commandName); // holds bookmark arguments to bookmarkExecute
            removeChars(strToExecute, '"'); // removing quotes from bookmark arguments
            isFound = 1; // if given index exists
        }
        temp = temp->next;
    }

    if (isFound == 0) { //If given index doesn't exist
        fprintf(stderr, "Bookmark not found with %d\n", indexOfNum);
        return;
    }

    char **argsStr = splitStr(strToExecute, ' '); // split the given string to args array to access them with their indexes
    char *argsToExec[128];
    int index = 0;

    while (argsStr[index] != NULL) { 
        argsToExec[index] = argsStr[index];
        index++;
    }

    argsToExec[index] = '\0';
    bookmarkExecute(argsToExec, &background);
    return;
}

char **splitStr(char *a_str, const char a_delim){
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *lastChar  = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    while (*tmp) {/* Count how many elements will be extracted. */
        if (a_delim == *tmp) {
            count++;
            lastChar  = tmp;
        }
        tmp++;
    }
    count += lastChar < (a_str + strlen(a_str) - 1); //Add space for trailing token.
    /* Add space for terminating null string so caller
    knows where the list of returned strings ends. */
    count++;
    result = malloc(sizeof(char *) * count);
    if (result) {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}

bool checkValidInt(const char *str){
    if (*str == '-'){
        ++str;
        return false;
    }

    if (!*str) return false; // if string is empty 

    while (*str) { 
        if (!isdigit(*str)) return false; //checking if entered string is digit
        else ++str;
    }

    return true;
}

// this function will search string in given directory recursively
void recursiveSearch(char *path, char *str){
    char *fileNameArr;
    int i = 0;
    DIR *d = opendir(path); // open directory with given path

    if (d == NULL) return; 
    
    struct dirent *dir; // for the directory entries
    
    while ((dir = readdir(d)) != NULL){ // checking whether current directory is readable
        if (dir->d_type != DT_DIR){ // gives a number for the type compare it with constant DT_DIR
            fileNameArr = dir->d_name;
            char dir_path[257];
            sprintf(dir_path, "%s/%s", path, dir->d_name);
            
            if (strstr(fileNameArr, ".c") || strstr(fileNameArr, ".C") 
                || strstr(fileNameArr, ".h") || strstr(fileNameArr, ".H")){
                printf("\n./%s ->\n", fileNameArr);
                if (searchInFile(dir_path, str) == -1){ //search given string in file 
                    fprintf(stderr, "File couldn't found.");
                }
            }
        }else if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0){ // if it is a directory 
            char direc_path[257];
            sprintf(direc_path, "%s/%s", path, dir->d_name);
            recursiveSearch(direc_path, str); // call to itself since search is recursive
        }
    }
    closedir(d); // Close file when searching is done
}

// search a string in given file
int searchInFile (char *fname, char *str){
    FILE *fp;
    int numOfLines = 1;
    int numOfResults = 0;
    char temp[1024]; // max char per line in file

    if ((fp = fopen(fname, "r")) == NULL) return -1; // try to open file to read

    while (fgets(temp, 1024, fp) != NULL){ 
        if ((strstr(temp, strdup(str))) != NULL){ // compares current line with given string
            printf("\t%d: ", numOfLines);
            printf("%s\n", temp);
            numOfResults++;
        }
        numOfLines++;
    }

    if (numOfResults == 0)fprintf(stderr, "\tNo match found.\n"); // if there is no match in current file

    if (fp)fclose(fp); // Close file when searching is done

    return 0;
}

// removes given char from string
void removeChars(char *str, char c){
    char *pr = str, *pw = str;
    while (*pr){
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

// search given string
int search(char *args[], int *background) {
    if (!strcmp(args[0], "search")){
        if (args[1] == NULL){ // input validation
            fprintf(stderr, "Input is not valid.\n");
            return 0;
        }
        if (!strcmp(args[0], "search") && !strcmp(args[1], "-r")){ // if second argument contains '-r' do recursive search   
            if (args[2] == NULL){ // input validation
                fprintf(stderr, "Input is not valid.\n");
                return 0;
            }
            
            char *argsEntered[128]; // args array contains arguments entered
            int index = 2; // getting index of word to search
            char searchStr[128];
            strcpy(searchStr, "");
            
            while (args[index] != NULL){
                removeChars(args[index], '"'); // removes quotes from given argument
                strcat(searchStr, args[index]); // storing non quote argument into string that is going to be searched
                strcat(searchStr, " ");
                index++;
            }

            argsEntered[0] = searchStr;
            char cwd[1024];
            recursiveSearch(getcwd(cwd, sizeof(cwd)), searchStr); // search given string recursively in all subdirectories

        }else{ // if -r option is not provided, do non-recursive search
            char cwd[1024];

            if (getcwd(cwd, sizeof(cwd)) == NULL){ // if directory not found
                fprintf(stderr, "getcwd() error");
            }

            // below search process is same with recursive part
            char *fileArr;
            DIR *dir;
            int i = 0;

            int index = 1;
            char searchstring[128];
            strcpy(searchstring, "");
            while (args[index] != NULL){
                removeChars(args[index], '"'); // removes quotes from given argument
                strcat(searchstring, args[index]); // storing non quote argument into string that is going to be searched
                strcat(searchstring, " ");
                index++;
            }
            struct dirent *readDir; // for read usage  
            if ((dir = opendir(cwd)) != NULL){
                while ((readDir = readdir(dir)) != NULL){
                    fileArr = readDir->d_name;
                    if (strstr(fileArr, ".c") || strstr(fileArr, ".C") 
                        || strstr(fileArr, ".h") || strstr(fileArr, ".H")){
                        printf("\nSearching in file: %s \n", fileArr);
                        if (searchInFile(fileArr, searchstring) == -1){
                            fprintf(stderr, "File not readable");
                        }
                    }
                }
                closedir(dir);
            }else{
                fprintf(stderr, "opendir error");
            }
            return 0;
        }
    }
}

// This function handles background processes
void backgroundProcesses(){
    if(bgProcesses.bgProcessCount == 0) return; // if there is no background process currently, return to caller.
        
    int status;
    pid_t exitedProcess;

    for(int i = 0; bgProcesses.bgProcessCount > 0 && i < MAX_BG_PROCESS; i++){
        if(bgProcesses.processpid[i] != 0){ // if there is a background process with nonzero pid
            exitedProcess = waitpid(bgProcesses.processpid[i], &status, WNOHANG); // return even if child is not avaliable
            if(exitedProcess > 0){ // Wait for the child whose process ID is equal to the value of pid
                fprintf(stderr, "\nThe process with the p_id = %d and the name = \"%s\" is terminated\n",
                bgProcesses.processpid[i], bgProcesses.processName[i]);

                bgProcesses.processpid[i] = 0;
                bgProcesses.bgProcessCount--;
            }else { // Wait for any child process
                printf("\nThe process with the p_id = %d and the name = \"%s\" is running\n",
                bgProcesses.processpid[i], bgProcesses.processName[i]);
            }
        }
    }
}

// This function sets the input and output flags
void ioRedirection(char *args[]){
    for(int i = 0; i < numOfArgs; i++){ // Iterate through given arguments
        if(strcmp(args[i], "<") == 0){ // If current argument contains input redirection
            if(i + 1 >= numOfArgs){ // Whether input file name is provided or not
                fprintf(stderr, "Provide a proper file name.\n");
                args[0] = NULL;
                return;
            } 
            
            inputFlag = 1; // Set input redirect flag to 1 since we are working on input redirection
            strcpy(inputName, args[i+1]); // copy given file name 
            
            // In case if we have output redirection following to input redirection 
            // Set following output part args such as '>' and 'output name'
            for(int j = i + 2, k = i; j < numOfArgs; j++, k++)
                args[k] = strdup(args[j]); // setting arguments for next iteration.
                 
            numOfArgs -= 2; // decreasing argument count since we move it by 2 argument.
            i--; 
        }else if(strcmp(args[i], ">") == 0 // If current argument contains output redirection
                || strcmp(args[i], ">>") == 0 
                || strcmp(args[i], "2>") == 0){

            if(i + 1 >= numOfArgs){ // Whether output file name is provided or not
                fprintf(stderr, "Provide a proper directory name.\n");
                args[0] = NULL;
                return;
            }

            outputFlag = 1; // Set output redirect flag to 1 since we are working on input redirection

            strcpy(outputSymbol, args[i]); // setting redirection type to given variable
            strcpy(outputName, args[i+1]); // copy given file name 
            numOfArgs -= 2;
            i--;
        }
    }
    // define last cell as null.
    args[numOfArgs] = NULL;
}

// Stop the currently running foreground process , and it's childs when ctrl + Z is pressed
void signalToStopHandler(){
    if(foregroundPid != 0){ // Checks any background process is running

        int status; //pointer to the location for returning a status
        pid_t exited_process = waitpid(foregroundPid, &status, WNOHANG); // return even if child is not avaliable

        if(exited_process < 0){ // Wait for any child process
            foregroundPid = 0; // process is already stopped
        }else{ // Wait for the child whose process ID is equal to the value of pid
            kill(foregroundPid, SIGTSTP); // stop the running process
            // SIGTSTP (for signal - terminal stop) may also be sent through driver by a user typing on a keyboard, usually Control-Z.
            
            // Check if we reach to maximum background process number
            if(bgProcesses.bgProcessCount == MAX_BG_PROCESS){
                fprintf(stderr, "No space left in background! \n");
                kill(foregroundPid, SIGCONT); // SIGCONT continues a process previously stopped by SIGSTOP or SIGTSTP.
            }else{ //  available space exists to add a new background process
                for(int i = 0; i < MAX_BG_PROCESS; i++){
                    if(bgProcesses.processpid[i] == 0){
                        // Adding new process to background processes with id and name
                        bgProcesses.processpid[i] = foregroundPid;
                        strcpy(bgProcesses.processName[i], fgProcessName);
                        bgProcesses.bgProcessCount++;
                        // Set the foreground process group ID associated with the terminal to main shell.
                        tcsetpgrp(STDIN_FILENO, getpgid(mainProcessPID));
                        tcsetpgrp(STDOUT_FILENO, getpgid(mainProcessPID));
                        // STDIN_FILENO is the default standard input file descriptor number which is 0.

                        printf("\nStopped process : \"%s\"\n", fgProcessName);
                        foregroundPid = 0; // set foreground process id as 0
                        break;
                    }
                }
            } 
            printf("\n");
            return;
        }
    }

    // If there is no foreground process just move to newline.
    printf("\nmyshell: ");
    fflush(stdout); // Forcing a flush guarantees that the prompt will be visible before blocking
}

// Splits the full path into array cells
void parsePath(){
    char *pathVar = getenv("PATH"); // get full path name into pathVar 
    // If path length  is not NULL, we will continue to split the path name into paths array.
    while(1){
        paths[pathLength] = splitText(pathVar, ':', pathLength); // split the path by :

        // if there is still argument in path, length will be increased by one
        if(paths[pathLength] != NULL)
            pathLength++;
        // there is no argument left to split
        else
            break;
    }

    char cwd[MAX_LINE];
    getcwd(cwd, sizeof(cwd)); // will hold the current working directory of myshell

    paths[pathLength] = strdup(cwd); // duplication of current working directory string
    pathLength++;
}

void setup(char inputBuffer[], char *args[], int *background){

    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
        
    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
    which is the user's screen in this case. inputBuffer by itself is the
    same as &inputBuffer[0], i.e. the starting address of where to store
    the command that is read, and length holds the number of characters
    read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
    However, if this occurs, errno is set to EINTR. We can check this  value
    and disregard the -1 value */

    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	    exit(-1);           /* terminate with error code of -1 */
    }

    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */
        switch (inputBuffer[i]){

            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;
            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];     
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;
            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';

                    i++; // making ineffective "&"" for next process
                    start = i;
                }
	    }/* end of switch */
    }/* end of for */

    args[ct] = NULL; /* just in case the input line was > 80 */
    numOfArgs = ct; // assign total number of arguments
    
} /* end of setup routine */

int main(void){
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    int returnedCommand; // checks validation of given command 
 
    mainProcessPID = getpid(); // Get id for main process
    parsePath(); // Get path of cwd as array
    signal(SIGTSTP, signalToStopHandler);  // handle stop signal for ctrl + z

    // Initialize all background and foreground processes as 0
    for(int i = 0; i < MAX_BG_PROCESS; i++) bgProcesses.processpid[i] = 0;
    
    while(1){
        background = 0; // if user doesn't provide &
        inputFlag = 0; // flag for input redirection
        outputFlag = 0; // flag for output redirection

        printf("myshell: ");
        fflush(stdout); // Forcing a flush guarantees that the prompt will be visible before blocking

        setup(inputBuffer, args, &background); /*setup() calls exit() when Control-D is entered */
  
        ioRedirection(args); // handle input output redirection

        if(args[0] == NULL) continue; // Waiting for user to enter an argument 

        returnedCommand = commandArgs(args); // 0 for a command, 1 for exit, 2 for not valid commandArgs

        if(returnedCommand == 1) break; // exit command entered
        else if(returnedCommand == 2) createNewProcess(args, background); // Argument is not an internal command.
        backgroundProcesses(); //In each iteration, we need to check the states of background processes.
    }
    freePath();
}
# myShell
- This program provides functionality for basic shell commands including special and built-in commands.

* gedit / gedit & usage : <br/>
→  If given command consists '&', desired task will be completed in background, in other words our shell will not wait for corresponding task to complete and it will immediately prompt the user for another command.<br/>
→  On the other hand, when given command doesn't contain '&', it will run in foreground, which means our shell will wait for current task.<br/>

## Some of built-in commands
* ps_all : <br/>
→ ps_all command will list every background instruction whether it is terminated or not and informs users accordingly.
 
 * ^Z :  <br/>
→ ctrl + z mechanism will stop the currently running foreground process and child processes that are forked.

 * search :  <br/>
→ Command for searching takes a string that is going to be searched and searches this string in all the files under the current directory and prints their line numbers, filenames and the line that the text appears.  <br/>
→ If -r option is used, the command will recursively search all the subdirectories as well. 

* bookmark :  <br/>
→ bookmarks frequently used commands. <br/>
→ Using -l (lowercase letter L) lists all the bookmarks added to myshell.<br/>
→ Bookmark -i idx executes the command bookmarked at index idx.<br/>
→ Using -d idx deletes the command at index idx and shifts the successive commands up in the list.<br/>

* exit :  <br/>
→ Terminate your shell process. 
→ Waits for every background process to finish their job.

## I/O Redirection part
*  myshell: myprog [args] > file.out  <br/>
→ Writes output of myprog into file.out.  <br/>
* myshell: myprog [args] >> file.out  <br/>
→ Appends output of myprog to the file.out.  <br/>
* myshell: myprog [args] < file.in <br/>
→ Uses contents of the  file.in as the standard input to myprog. <br/>
* myshell: myprog [args] 2> file.out <br/>
→ Writes the  error of myprog to the file.out. <br/>
* myshell: myprog [args] < file.in > file.out  <br/>
→ Reads input from file.in and output of the command is directed to the file.out <br/>

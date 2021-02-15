# Application Installation Instructions
APG Version 7.0 has been developed on a the Linux Ubuntu 20.04 operating system using the [Eclipse for C/C++](https://www.eclipse.org/downloads/packages/) IDE. Additionally, it has been tested on Windows 10 using Eclipse and [git for windows](https://gitforwindows.org/). The installation instructions given here use the Ubuntu command line and the Eclipse IDE. Following along with another Linux distro or IDE with which you are familiar should not be difficult.


We will assume that the installation is being done in the home directory, but you can choose any other directory that is convenient. From your home directory, `/home/me`, install the APG Version 7.0 repository.
> git clone https://github.com/ldthomas/apg-7.0.git<br>
> cd apg-7.0

Alternatively, download and unzip `apg-7.0.zip`. (The file directory may be slightly different, `apg-7.0-main`, for example.)

In the Eclipse IDE switch the work space.
> File->Switch Workspace->other 

 and select `/home/me/apg-7.0`.

Open the new project dialog with
>File->new->C/C++ Project->C Managed Build

In "project name" type `apg` and uncheck the box "use default location". Click "Browse" and select
>/home/me/apg-7.0/apg

The project type should be "Empty Project" and the toolchain should be "Linux GCC". Click "Finish". The project is now created.

Next it needs to be configured with the necessary macros and source libraries. With the project name selected navigate to
>File->Properties->C/C++ General->Paths and Symbols

From the "Symbols" tab click "Add" and type `APG_AST`. Check the box "Add to all configurations" and click "OK". From the "Source Location" tab click "Link folder".  In the "Folder name" box type `api`, check the box "Link to folder in the file system" and navigate to
>/home/me/apg-7.0/api

and click "OK". Repeat for
>library<br>
>/home/me/apg-7.0/library

and
>utilities<br>
>/home/me/apg-7.0/utilities

At this point the project is ready to build. Right click and select `BuildConfigurations->Set Active->Release`. Right click the project name and select `Build Project`. It should build with no errors or warnings. The APG command-line parser generator will be found in 
>/home/me/apg-7.0/apg/Release/apg

For convenient access, it could now be copied to a directory on the path, `/usr/bin` for example, but on some Linux distros there is a conflict with the pre-installed  Automatic Password Generator, also named `apg`. Other possibilities are to rename it and copy it to a path directory or create an alias in the `.bashrc` or `.bash_aliases` files if they exist.


﻿#  Manual Installation Instructions
﻿
APG Version 7.0 has been developed on a the Linux Ubuntu 20.04 operating system using the [Eclipse for C/C++](https://www.eclipse.org/downloads/packages/) IDE version 4.16. Additionally, it has been tested on Windows 10 using Eclipse and [git for windows](https://gitforwindows.org/). The manual installation instructions given here work equally well with the Ubuntu command line and Windows using the git-for-windows command line. 

Install the APG Version 7.0 repository and, to keep the original source directories clean, create a `build` directory for the IDE project files.
>$git clone https://github.com/ldthomas/apg-7.0.git<br>
> $cd apg-7.0<br>
> $mkdir build<br>
> $pwd<br>
> /my/path/apg-7.0

Hereafter, `./` refers to `/my/path/apg-7.0`

Open the Eclipse IDE. The workspace location is not important, but it is probably good practice to  switch the work space to the `./build` directory. Open
> File->Switch Workspace->other 

 and select `./build`.

Open the new project dialog with
>File->new->C/C++ Project->C Managed Build

In `project name` type `apg70` and uncheck the box `use default location`. In the `Location` window type (full path name)
>./build/apg70

The project type should be `Empty Project` and the toolchain should be `Linux GCC` (or `MinGW GCC` for Windows). Click `Finish`. The project is now created.

Next it needs to be configured with the necessary macros and source libraries. Right click on the project name and navigate to
>Properties->C/C++ General->Paths and Symbols

From the `Symbols` tab click `Add` and type `APG_AST`. Check the box `Add to all configurations` and click `OK`. From the `Source Location` tab click `Link folder`.  In the `Folder name` box type `apg`, check the box `Link to folder in the file system` and navigate to
>./apg

click `OK`. Repeat for
>api<br>
>library<br>
>utilities

At this point the project is ready to build. Right click the project and select `BuildConfigurations->Set Active->Release`. Right click the project name again and select `Build Project`. It should build with no errors or warnings. The APG command-line parser generator will be found in 
>./build/apg70/Release/apg70

For convenient access, it could now be copied to a directory on the path. e.g.
>$sudo cp ./build/Release/apg70 /usr/local/bin


_Note: Naming the application `apg70`, rather than simply `apg`, avoids conflict with the Automated Password Generator application, also named `apg`, installed by default on the Ubuntu 20.04 Linux distro._

Projects for all of the example applications can be created with a similar process.


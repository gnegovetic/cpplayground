# Build and debug C++ code using Docker

## Introduction
We want to build our code using on a specific Linux version and a specific compiler version, and then debug the code using VS Code.

## Build the container (see docker/Dockerfile)
From the folder root: ``` docker build -t hello-world-container docker/ ```

Start the container: ``` docker run --name hello-world-builder -it -v $(pwd):/HelloWorld hello-world-container ```

The above command will: start the container in interactive mode, map our source folder from the host to a workdir folder inside the container. We could copy our source files to inside the container, which is OK for small projects, but not practical for real project -- and it would require us to rebuild the container every time the source code changes. 
Note: you can provide ``` --user "$(id -u):$(id -g)" ```, to use the current user to create folders and files (during the build process) so they are not owned by root (which would be the default). This is very convenient to get to the files from the host, but VS Code doesn't attach to it by default for debugging. 

If the run command is sucessful, this will start a bash terminal inside your container.

## Build our code with the container
Once inside the bash terminal of the container, we need to build our code: 
  ```  
    mkdir build && cd build
    cmake ..
    make 
  ```  

Note: If you lose your container terminal, you can get back by attaching to it: docker attach hello-world-builder

You can always stop and start the container with: docker stop/start hello-world-builder

## Debugging using VS Code
To set up the debugging, first install "Remote - Containers" extension. Then simply attach VS Code to your running container: click lower-left green connect button > Attach to running container > select 'hello-world-builder' container.

Now we need to tell GDB which app we want to debug. Select Run > Add Configuraiton... > select (gdb) launch. 
Then change Program tag to be: "program": "/HelloWorld/build/helloWorld",
Now set a break point in the CPP file and click Run > Start Debugging (F5)

Enjoy... 

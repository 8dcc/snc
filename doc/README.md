**Simple NetCat.**


# Table of Contents

1.  [Description](#orgbb1853c)
2.  [Building](#orgaed1185)
3.  [Usage](#orgb460cfe)


<a id="orgbb1853c"></a>

# Description

Simple port of the NetCat command in C using sockets.


<a id="orgaed1185"></a>

# Building

    $ git clone https://github.com/8dcc/snc
    $ cd snc
    $ make
    ...


<a id="orgb460cfe"></a>

# Usage

    $ ./snc h
    Usage:
        ./snc h       - Show this help
        ./snc l       - Start in listen mode
        ./snc c <IP>  - Connect to specified IP address

In a terminal:

    $ ./snc l  # Start to listen. It will print recieved data
    
    $ ./snc l > output.txt

In another terminal:

    $ ./snc c IP
    Type stuff here!
    It will send it to the listening session.
    
    $ ./snc c IP < input.txt


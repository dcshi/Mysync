Mysync
======

Description
===========
Mysync is the role of a slaveDB. It recive the binlog from the master, and save them with json into <a href="http://fallabs.com/tokyocabinet/" title="tc" target="_blank">tokyocabinet</a>. So you can get your data form tc and sync them to any other servers, such as redis, mc, sphinx and so on. As an option, <a href="https://code.google.com/p/httpsqs/" title="httsqs" target="_blank">httpsqs</a> is recommended. So that, PHP, PTYHON, C or any language which supports http protocol is OK.

Run Mysync
==========
1)the master-mysql should be configured correctly.(binlog_do_db,binlog_format etc.) <br/>
2)In the example file. example.sql and test.sql is provided. With the example.sql, a test table would be created easily, it also include all column types which are supported so far. test.sql include a InsertSQL<br/>
3)run command like: ./mysync -c etc/ms.cf<br/>
4)insert a record into the test-table.<br/>
5)if some information is showed on your screen, that is ok.

Dependencies
============
1) <a href="http://fallabs.com/tokyocabinet/" target="_blank" title="tc">tokyocabinet</a> is needed. Its installation is simple.(./configure [--enable-off64];make;make install) <br/>
2) libmysqlclient-dev is needed.

Installation
============
make;make install;
make db_tool;

TODO
====

Author
======
dcshi(施俊伟) <dcshi@qq.com>

Copyright and License
=====================
This module is licensed under the BSD license.
Copyright (C) 2013, by dcshi(...). <dcshi@qq.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and 
      the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 

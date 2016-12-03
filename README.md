# README #

2015-05-12
支持指令：
"create" 			开启新的编码任务。
	该指令的值为下面几种
	hardware		硬件板卡输出
	savefile		文件录制
	rtmp				RTMP流推送
	rtp					RTP流推送
	
"destroy"			销毁编码任务
	无值
	
"inifile"			配置文件全路径
"name"				编码任务名称，唯一
"bye"					进程退出。

范例:
创建文件录制编码任务,该任务名称为test1
silentEncoder.exe --create=savefile --name=test1 --inifile="G:\\git\\silentEncoder\\bin\\x64\\Debug\\record.ini"

销毁文件录制编码任务,被销毁的任务名称为test1
silentEncoder.exe --destroy --name=test1



This README would normally document whatever steps are necessary to get your application up and running.

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact
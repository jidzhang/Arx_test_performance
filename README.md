这个工程用于测试、对比操作AutoCAD图形数据库的三种不同方式的时间消耗，
是文章“AutoCAD二次开发技术研究及其在剪力墙软件中的应用”中综合应用实例的完整解决方案。

测试代码主要位于test_performanceCommands.cpp文件。

本工程用Visual Studio 2010编写，可以编译AutoCAD2002和2008两个版本。
本工程依赖ObjectARX开发包，需要到Autodesk官方网站下载。

使用方法：

1. 编译后生成**test_performance.arx**
2. 打开AutoCAD2008或2002，先输入ARX命令加载刚才生成的arx，然后在CAD中输入**test**即可执行测试程序。
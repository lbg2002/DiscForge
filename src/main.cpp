#include "window.h"
#include "version.h"
#include <cstdio>
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    QCoreApplication::setApplicationName("DiscForge");
    QCoreApplication::setOrganizationName("DiscForge");
    QCoreApplication::setApplicationVersion(DISCFORGE_VERSION);
    const auto args=app.arguments();
    if(args.contains("--self-test")) {
        QTemporaryDir testDir;
        qputenv("XDG_CONFIG_HOME",testDir.path().toUtf8());
        qputenv("XDG_DATA_HOME",testDir.path().toUtf8());
        QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,testDir.path());
        Window window;
        const int at=args.indexOf("--render");
        bool ok=window.selfTest(at>=0&&at+1<args.size()?args[at+1]:QString());
        std::puts(ok?"界面检查通过":"界面检查失败");
        return ok?0:1;
    }Window window;
    window.show();
    return app.exec();
}

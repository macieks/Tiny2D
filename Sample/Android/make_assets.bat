@echo ===========================
@echo === Clearing assets directory ===
@echo ===========================
rmdir /S /Q assets\

@echo ===============================
@echo === Copying new assets ===
@echo ===============================
xcopy /Y /S ..\..\Data\*.* assets\
xcopy /Y /S ..\..\Sample\data\*.* assets\

@pause
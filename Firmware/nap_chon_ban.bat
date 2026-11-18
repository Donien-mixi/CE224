@echo off
setlocal
title CHON BAN FIRMWARE DE NAP
:menu
cls
echo ============================================================
echo  CHON BAN FIRMWARE DE NAP (nap CA HAI board)
echo ============================================================
echo   1. RT38-CONT / NET38    - hien hanh: doi huong lien tuc, het khung
echo   2. RT37-SMOOTH / NET37  - truoc:     xuat phat muot
echo   3. RT34-CORNER / NET35  - truoc:     50/80/90, cua 0.80, da co log 173 s
echo   4. RT29-NOCUT / NET30   - du phong:  50/72/80, khong bu banh trai
echo   Q. Thoat
echo ============================================================
set /p CH=Chon 1/2/3/4/Q:
if /i "%CH%"=="1" ( call "%~dp0RT38_CONT_20261118\nap_rt38.bat" & goto :menu )
if /i "%CH%"=="2" ( call "%~dp0RT37_SMOOTH_20261118\nap_rt37.bat" & goto :menu )
if /i "%CH%"=="3" ( call "%~dp0RT34_CORNER_20261117\nap_rt34.bat" & goto :menu )
if /i "%CH%"=="4" ( call "%~dp0RT29_NOCUT_20261115\nap_rt29.bat" & goto :menu )
if /i "%CH%"=="Q" goto :eof
goto :menu

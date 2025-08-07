@echo off

set date=%DATE%
set time=%TIME%

git add .
git commit -m "updated %date% %time%"
git push origin main
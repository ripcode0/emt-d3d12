@echo off

set d=%DATE%
set time=%TIME%

git add .
git commit -m "updated %d% %time%"
git push origin main
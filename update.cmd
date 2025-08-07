@echo off

set time=%TIME%

git add .
git commit -m "updated %time%"
git push origin main
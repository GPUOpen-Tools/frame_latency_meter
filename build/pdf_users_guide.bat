REM Convert markdown to PDF, Internal batch file not for open source
REM Requirements 
REM pandoc: https://github.com/jgm/pandoc/releases (tested with 3.1.13)
REM Miktex: https://miktex.org/download for Windows (used basic-miktex-24.1-x64.exe)
REM Ref for this cmd line: https://pavolkutaj.medium.com/markdown-to-pdf-with-pandoc-and-miktex-58b578cedf4b
set CurrDir=%CD%
for %%* in (.) do set CurrDirName=%%~nx*
cd ../documentation
pandoc -V geometry:"top=2cm, bottom=1.5cm, left=2cm, right=2cm" -f markdown-implicit_figures -o users_guide.pdf users_guide.md
cd %CurrDir%

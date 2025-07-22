@echo off
setlocal enabledelayedexpansion

REM Compilar o programa
echo Compilando o programa IBEA MOTSPPP...
@REM g++ -O3 -o ibea_motsppp.exe index.cpp
g++ -o ibea_motsppp.exe index.cpp structures.cpp BoundedParetoSet.cpp

if %ERRORLEVEL% EQU 0 (
    echo Compilação bem-sucedida!
    
    REM Lista de tamanhos de instâncias
    for %%t in (5 8 10 20 50 100 200) do (
        REM Executar para instâncias simétricas
        for %%i in (1 2 3 4 5 6) do (
            @REM echo Executando instância %%t.%%i simétrica...
            if exist ..\resultados\ibea\a3\sym\%%t.%%i rmdir /s /q ..\resultados\ibea\a3\sym\%%t.%%i
            mkdir ..\resultados\ibea\a3\sym\%%t.%%i
            ibea_motsppp.exe ..\instances\A3\symmetric\%%t.%%i.in ..\resultados\ibea\a3\sym\%%t.%%i\dummy.txt
        )
        
        REM Executar para instâncias assimétricas
        for %%i in (1 2 3 4 5 6) do (
            echo Executando instância %%t.%%i assimétrica...
            if exist ..\resultados\ibea\a3\asym\%%t.%%i rmdir /s /q ..\resultados\ibea\a3\asym\%%t.%%i
            mkdir ..\resultados\ibea\a3\asym\%%t.%%i
            ibea_motsppp.exe ..\instances\A3\asymmetric\%%t.%%i.in ..\resultados\ibea\a3\asym\%%t.%%i\dummy.txt
        )
    )
    
    del ibea_motsppp.exe
) else (
    echo Erro na compilação.
)

echo Todas as execuções foram concluídas!
@REM pause
@echo off
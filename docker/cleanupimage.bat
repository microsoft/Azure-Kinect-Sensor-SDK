:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

ECHO Stopping k4a
docker stop k4a > NUL
ECHO Removing k4a
docker rm k4a > NUL

ECHO Removing volumes
docker volume rm k4a-src
docker volume rm k4a-build


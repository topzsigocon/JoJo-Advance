# Butano project Makefile (based on the official Butano template)
TARGET       := jojo_allstar_gba
BUILD        := build
LIBBUTANO    := ../butano
PYTHON       := python3
SOURCES      := src
INCLUDES     := include
DATA         :=
GRAPHICS     := graphics
AUDIO        := audio
AUDIOBACKEND := null
AUDIOTOOL    :=
DMGAUDIO     := dmg_audio
DMGAUDIOBACKEND := null
ROMTITLE     := JOJO ALLSTAR
ROMCODE      := JJBA
USERFLAGS    :=
USERCXXFLAGS :=
USERASFLAGS  :=
USERLDFLAGS  :=
USERLIBDIRS  :=
USERLIBS     :=
DEFAULTLIBS  :=
STACKTRACE   :=
USERBUILD    :=
EXTTOOL      :=

ifndef LIBBUTANOABS
export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak

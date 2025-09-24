# !/bin/bash

# Aliases for RIX tools
alias rixhub="$HOME/.rix/bin/rixhub"
alias rixmsg="$HOME/.rix/bin/rixmsg"
alias rixinfo="$HOME/.rix/bin/rixinfo"

# Set the CMake prefix path for RIX
case ":$CMAKE_PREFIX_PATH:" in
  *":$HOME/.rix:"*) ;;
  *) export CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH:$HOME/.rix" ;;
esac

# Source the RIX Python virtual environment
source $HOME/.rix/venv/bin/activate

# Edit these to change the default IP and port of RIXHub and RIX nodes
export RIX_RIXHUB_IP=127.0.0.1
export RIX_RIXHUB_PORT=48104
export RIX_DEFAULT_IP=127.0.0.1
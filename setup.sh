# sudo add-apt-repository universe
sudo apt update
sudo apt install build-essential cmake -y
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev -y
sudo apt install xorg-dev libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev mesa-utils -y
sudo apt install curl -y
curl -Ls https://astral.sh/uv/install.sh | sh
grep -qxF 'export PATH="$HOME/.local/bin:$PATH"' ~/.bashrc || \
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

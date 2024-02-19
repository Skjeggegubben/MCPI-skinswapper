# MCPI-skinswapper
Skin mod for MCPI, lets you swap between skins realtime in game with arrow keys


This is a mod for MCPI-reborn, put "libskinswapper.so" in your mods dir and start up the game, and it
lets you change skin instantly by swapping between different skin files in your ~/.minecraft-pi/SKIN_DIR/


In order for it to work completely, uncheck the checkbox "Load Custom Skins" at game startup.
Disabling the game's default implemented skin handling is needed because it overrides this trickery.


For showcasing, the mod comes with two example skins as default to swap between - Spiderman and Default-StevePi
so that you can hit the arrow key at any moment to become Spiderman and go jumping and climing etc :) 


That is just an example, as you can add the skins you want, even a series of the same skin with just
different facial expressions, to show your emotions, or different color hair, thieves mask for stealings etc..
Use your imagination :) 


<b>Also:</b> You don't need to <b>exit and restart</b> game to delete a skin file or drop a new one in folder! <b>Just do it!</b>


<b>But does this work when you're on a server?</b> - Yes ofc it does <i>if the server also has the mod</i> :D 
It works ofc on both dedicated server, and normal game client as server! ;) Other users see your skin swap instantly!

<h2>HAVE FUN :)</h2>

<b>If you want to compile by yourself</b>, you should know I'm using an older version 
sdk than the newest, and it is ofc provided in file "sdk_2.4.8.zip", just unzip to 
~/minecraft-pi/ OR wherever you want and edit accordingly in CMakeList.txt!<br><br>

Beware that if you want to compile with a newer sdk, you need to change a lot of the code!<br><br>

<h3>Compile your own and smoke it:</h3>

sudo apt install g++-arm-linux-gnueabihf

sudo apt install gcc-arm-linux-gnueabihf

mkdir build

cd build

cmake ..

make -j$(nproc)

cp libskinswapper.so ~/.minecraft-pi/mods


The file can also be stripped to a smaller size by doing this:

arm-linux-gnueabihf-strip libskinswapper.so

<h2>Big thanks to Bigjango, lot of code chunks are from his libextrapi and other mods! :D :pray: </h2>

# `fck`

`fck` is a plugin-based C (I want to say 99) engine focusing on game technology from an educational point of view.

Some plugins make sense; other plugins do not make sense. It is for the reader to decide. 
Conversation about these things is always valuable. 

The philosophy of this project is a "simple" out of the box experienece. 
The hardest part is installing Vulkan.
Besides that, users only need to open this project in VS2022/26/etc. as a folder, and this engine should pur like a cat.

For VS Code, more steps might be needed, but I should add a few more extensions to make it work, or maybe outline a little rundown.

`fck` got tested on macOS and Windows, but there has not been a good opportunity to test it on Linux with x11 or so. (Spoiler: It won't run)

Some interesting spots in there to look at are `db`, `ec`, `glsl-reflection`, `input-*`, `os`, `render-vk` and maybe `nuklear`.

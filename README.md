# XV6_CopyPaste_Functionality

To support the copy-paste functionality, the following functionalities have been added to xv6:

1. **Entry in Copy-Paste Mode:** Press SHIFT + ALT + C to enter the copy-paste mode.

2. **Text Selection and Copying:** While in copy-paste mode, use the W, A, S, or D keys to navigate the cursor. Press Q to start the selection mode and then use the navigation keys to select the text that needs to be copied. Once the desired text is selected, press E to exit the selection mode.

3. **Text Paste:** Press SHIFT + ALT + P to paste the copied text.

**Instructions to Start the Program:**

1. Open the terminal in the project's directory.
2. Run 'make clean' to clean the project.
3. Run 'make qemu' to start the xv6 operating system, displayed in the QEMU window.

**Copy-Paste Mode:**
- To enter copy-paste mode, press SHIFT + ALT + C.
- While in copy-paste mode, the text that the cursor passes over is colored black on white during cursor navigation and text selection.

**Text Selection and Copying:**
- Use the W, A, S, or D keys to navigate the cursor.
- To start the selection mode, press Q.
- Use the navigation keys (W, A, S, D) to select the text that needs to be copied.
- Once the desired text is selected, press E to exit the selection mode.

**Exit Copy-Paste Mode:**
- Press SHIFT + ALT + C to exit copy-paste mode.
- When exiting copy-paste mode, the cursor returns to the original position on the screen from which copy mode was started.

**Text Paste:**
- To paste the copied text, press SHIFT + ALT + P.

**Note:** Copy-paste mode enables the text over which the cursor passes to be colored black on white during cursor navigation and text selection.

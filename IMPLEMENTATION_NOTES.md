# Implementation Notes

## OFX API Considerations

### DrawSuite Availability
The plugin uses OFX DrawSuite for overlay rendering. Note that:
- DrawSuite support varies by host application
- Some hosts may not support all DrawSuite functions
- The plugin includes fallback behavior when DrawSuite is unavailable

### Parameter Persistence
All parameters are automatically persisted by the OFX host:
- Scan line endpoints (P1, P2) are saved with project files
- Data source selection is preserved
- Visualization settings are maintained

### Multi-Resolution Support
The plugin handles multi-resolution workflows:
- Normalized coordinates ensure scan lines work at any resolution
- Automatic scaling when resolution changes
- GPU sampling adapts to image dimensions

## GPU Implementation Details

### Metal (macOS)
- **Direct GPU Image Sharing**: On macOS with DaVinci Resolve, Metal enables zero-copy memory access
- **Kernel Compilation**: Metal kernels are compiled at build time into `.metallib`
- **Runtime Loading**: The plugin loads the embedded `.metallib` from bundle resources
- **Device Selection**: Uses system default Metal device (typically integrated or discrete GPU)

### OpenCL (Cross-Platform)
- **Kernel Loading**: OpenCL kernels are loaded from resource files at runtime
- **Platform Detection**: Automatically detects available OpenCL platforms (NVIDIA, AMD, Intel)
- **Device Fallback**: Falls back to CPU device if no GPU is available
- **Kernel Compilation**: Kernels are compiled at runtime by OpenCL driver

### Performance Tuning
- **Thread Group Size**: Metal uses 64 threads per group (optimal for most GPUs)
- **Sample Count**: GPU acceleration most beneficial for 256+ samples
- **Memory Access**: Bilinear sampling uses texture cache efficiently

## CPU Implementation Details

### Bilinear Interpolation
- **Sub-pixel Accuracy**: Provides smooth sampling between pixels
- **Boundary Handling**: Clamps coordinates to image bounds
- **Component Support**: Handles both RGB (3 components) and RGBA (4 components)

### Optimization Opportunities
- **SIMD**: Could be optimized with SSE/AVX for parallel processing
- **Threading**: Could parallelize sampling across multiple CPU cores
- **Caching**: Could cache samples for static scan lines

## Coordinate System

### Normalized Coordinates
- **Range**: [0.0, 1.0] for both X and Y
- **Origin**: Top-left corner (0, 0)
- **Resolution Independence**: Works at any image resolution
- **Conversion**: Pixel coordinates = normalized × image dimension

### Pixel Coordinates
- **Range**: [0, width-1] for X, [0, height-1] for Y
- **Origin**: Top-left corner (0, 0)
- **Sub-pixel**: Floating-point values for bilinear interpolation

## Data Source Modes

### Input Clip (Mode 0)
- Samples the primary video clip at the plugin's position in the node graph
- **Use Case**: Analyze color transforms and LUTs applied before this node
- **Post-LUT Analysis**: Shows intensity after all previous effects

### Auxiliary Clip (Mode 1)
- Samples an optional secondary input clip
- **Use Case**: Compare two different sources or versions
- **Requirement**: Auxiliary clip must be connected

### Built-in Ramp (Mode 2)
- Generates a linear 0-1 grayscale signal mathematically
- **Use Case**: Test LUTs and transfer functions independently of source footage
- **LUT Testing**: Apply LUT to this node and observe transfer function response

## Rendering Pipeline

### Render Steps
1. **Fetch Images**: Get source and output images from OFX host
2. **Copy Source**: Copy source image to output (if different)
3. **Sample Intensity**: 
   - Select data source
   - Sample along scan line (GPU or CPU)
   - Generate samples for R, G, B channels
4. **Render Overlay**:
   - Draw reference ramp (if enabled)
   - Draw grid overlay
   - Draw RGB curves
5. **Complete**: Return rendered image to host

### DrawSuite Usage
- **Begin/End Draw**: Wraps all drawing operations
- **Color Setting**: Sets color before each drawing operation
- **Line Drawing**: Used for curves, grid, and scan line
- **Rectangle Drawing**: Used for plot border

## Error Handling

### GPU Fallback
- If GPU sampling fails, automatically falls back to CPU
- No user-visible error; seamless transition
- Performance degradation is the only indicator

### Image Access
- Validates image pointers before use
- Checks image bounds and component count
- Handles missing or invalid clips gracefully

### Parameter Validation
- Clamps normalized coordinates to [0, 1]
- Validates sample count range (64-2048)
- Ensures plot height is within bounds

## Host Compatibility

### DaVinci Resolve
- **Tested Versions**: 17+, 18+
- **Metal Support**: Full Direct GPU Image Sharing on macOS
- **DrawSuite**: Full support for overlay rendering
- **OSM**: Full On-Screen Manipulator support

### Other OFX Hosts
- **Compatibility**: Should work with any OFX 1.4+ host
- **Feature Support**: May vary by host implementation
- **Testing**: Recommended to test with target host before deployment

## Future Enhancements

### Potential Optimizations
1. **Async GPU Operations**: Overlap sampling with image copy
2. **Sample Caching**: Cache samples for static scan lines
3. **Progressive Rendering**: Lower quality during scrubbing, full quality on pause
4. **Multi-threaded CPU**: Parallelize CPU sampling across cores

### Feature Additions
1. **Multi-line Support**: Multiple scan lines with comparison
2. **Temporal Analysis**: Track intensity changes over time
3. **Export Functionality**: Save profile data to file
4. **Custom Curves**: Add luminance, saturation, hue curves
5. **3D Visualization**: Interactive color space plots

## Debugging Tips

### Enable Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Logging
- Use OFX `setPersistentMessage()` for error reporting
- Check host application logs for plugin messages
- Use debugger breakpoints in render functions

### GPU Debugging
- **Metal**: Use Xcode GPU Frame Capture
- **OpenCL**: Use vendor-specific debugging tools
- **Validation**: Test with CPU fallback to isolate GPU issues

### Common Issues
1. **Plugin Not Loading**: Check OFX SDK version compatibility
2. **OSM Not Appearing**: Verify interact descriptor registration
3. **No GPU Acceleration**: Check GPU availability and driver versions
4. **Rendering Errors**: Validate image format and component count

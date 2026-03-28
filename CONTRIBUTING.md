# Contributing to SkySweep32

First off, thank you for considering contributing to SkySweep32! 🎉

## Code of Conduct

This project adheres to a code of conduct. By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues. When creating a bug report, include:

- Detailed steps to reproduce
- Hardware configuration (ESP32 board, RF modules)
- Software version (commit hash)
- Serial output logs
- Expected vs actual behavior

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, include:

- Clear and descriptive title
- Detailed description of the proposed functionality
- Why this enhancement would be useful
- Possible implementation approach

### Pull Requests

1. Fork the repo and create your branch from `master`
2. If you've added code, test it on actual hardware
3. Ensure your code follows the existing style
4. Update documentation as needed
5. Write clear commit messages
6. Submit a pull request

## Development Setup

```bash
# Clone your fork
git clone https://github.com/YOUR-USERNAME/SkySweep32.git
cd SkySweep32

# Install PlatformIO
pip install platformio

# Build
pio run

# Upload to ESP32
pio run --target upload
```

## Coding Standards

- Use meaningful variable names
- Comment complex logic
- Follow existing code structure
- Test on hardware before submitting
- Keep commits atomic and well-described

## Hardware Testing

All code changes affecting RF modules must be tested on actual hardware:
- ESP32 DevKit
- CC1101 (900 MHz)
- NRF24L01+ (2.4 GHz)
- RX5808 (5.8 GHz)
- OLED display

## Legal Considerations

⚠️ **IMPORTANT**: Contributions involving RF transmission or countermeasures must:

1. Include appropriate legal warnings
2. Be disabled by default (compile-time flag)
3. Document regulatory requirements
4. Not encourage illegal use

## Documentation

- Update README.md for user-facing changes
- Update docs/ for technical changes
- Maintain bilingual documentation (EN/RU)
- Include code comments for complex logic

## Commit Messages

Use clear, descriptive commit messages:

```
Add CC1101 frequency hopping support

- Implement channel hopping algorithm
- Add configuration options
- Update documentation
```

## Questions?

Feel free to open an issue with the `question` label.

## License

By contributing, you agree that your contributions will be licensed under the GPL-3.0 License.

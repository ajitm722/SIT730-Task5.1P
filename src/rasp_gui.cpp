#include <QApplication> // Manages the Qt application and GUI event loop
#include <QWidget>      // Base class for all visual containers like windows
#include <QRadioButton> // Allows user to select one LED at a time
#include <QPushButton>  // Push button for exiting the application
#include <QVBoxLayout>  // Layout manager that arranges widgets vertically
#include <QLabel>       // Displays static text (used for the GUI title)
#include <QGroupBox>    // Visually groups related widgets (like LED controls)
#include <QFont>        // Provides font customization for widgets
#include <QPalette>     // Allows color customization (background, foreground)
#include <memory>       // For std::unique_ptr — smart pointer for safe memory handling
#include <stdexcept>    // For throwing runtime errors during GPIO setup
#include <pigpio.h>     // Raspberry Pi GPIO control library

// GPIO pin assignments for the three LEDs
constexpr int RED_LED{17};   // Connected to GPIO pin 17 (physical pin 11)
constexpr int GREEN_LED{27}; // Connected to GPIO pin 27 (physical pin 13)
constexpr int BLUE_LED{22};  // Connected to GPIO pin 22 (physical pin 15)

/**
 * @brief Initializes the pigpio library and sets up each LED pin as output.
 *        Throws a runtime_error if pigpio fails to initialize.
 */
void setupGpio()
{
    if (gpioInitialise() < 0)
    {
        throw std::runtime_error{"GPIO initialization failed"};
    }

    // Set each pin as output to control the LEDs
    gpioSetMode(RED_LED, PI_OUTPUT);
    gpioSetMode(GREEN_LED, PI_OUTPUT);
    gpioSetMode(BLUE_LED, PI_OUTPUT);
}

/**
 * @brief Turns ON the selected LED while turning OFF the other two.
 *        Ensures that only one LED is ON at a time.
 */
void turnOnOnly(int pin)
{
    gpioWrite(RED_LED, pin == RED_LED ? 1 : 0);
    gpioWrite(GREEN_LED, pin == GREEN_LED ? 1 : 0);
    gpioWrite(BLUE_LED, pin == BLUE_LED ? 1 : 0);
}

/**
 * @brief Creates the title label for the GUI window.
 *        Uses a smart pointer for temporary ownership during construction.
 *        Ownership is released later to Qt layout manager.
 */
std::unique_ptr<QLabel> createTitleLabel()
{
    auto label{std::make_unique<QLabel>("LED Controller Interface")};

    QFont font{"Arial", 16, QFont::Bold};
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { color: white; }"); // White text on black background

    return label;
}

/**
 * @brief Creates the LED selection controls: 3 radio buttons inside a QGroupBox.
 *        Each button is connected to a corresponding LED pin.
 *        Memory is transferred to Qt using .release() where applicable.
 */
std::unique_ptr<QGroupBox> createLedControls()
{
    // Smart pointers for each radio button
    auto redBtn{std::make_unique<QRadioButton>("Red LED")};
    auto greenBtn{std::make_unique<QRadioButton>("Green LED")};
    auto blueBtn{std::make_unique<QRadioButton>("Blue LED")};

    QFont btnFont{"Arial", 12};
    redBtn->setFont(btnFont);
    greenBtn->setFont(btnFont);
    blueBtn->setFont(btnFont);

    // Set colored text for better readability
    redBtn->setStyleSheet("QRadioButton { color: red; }");
    greenBtn->setStyleSheet("QRadioButton { color: green; }");
    blueBtn->setStyleSheet("QRadioButton { color: cyan; }");

    // Connect each button's clicked signal to turn on the correct LED
    QObject::connect(redBtn.get(), &QRadioButton::clicked, []()
                     { turnOnOnly(RED_LED); });
    QObject::connect(greenBtn.get(), &QRadioButton::clicked, []()
                     { turnOnOnly(GREEN_LED); });
    QObject::connect(blueBtn.get(), &QRadioButton::clicked, []()
                     { turnOnOnly(BLUE_LED); });

    // Layout to stack the buttons vertically
    auto layout{std::make_unique<QVBoxLayout>()};
    layout->addWidget(redBtn.get());
    layout->addWidget(greenBtn.get());
    layout->addWidget(blueBtn.get());

    // Group box to visually contain the buttons
    auto group{std::make_unique<QGroupBox>("Select LED to Turn On")};
    group->setFont(QFont{"Arial", 11});
    group->setStyleSheet("QGroupBox { color: white; }");

    // Important: Qt takes ownership of the layout and all its child widgets
    group->setLayout(layout.release());

    // Release smart pointers so they don't delete objects Qt now owns
    redBtn.release();
    greenBtn.release();
    blueBtn.release();

    return group;
}

/**
 * @brief Creates a styled Exit button that gracefully quits the app.
 *        Clicking it triggers QApplication::quit().
 */
std::unique_ptr<QPushButton> createExitButton()
{
    auto button{std::make_unique<QPushButton>("Exit")};
    button->setFont(QFont{"Arial", 12});
    button->setStyleSheet("QPushButton { background-color: grey; color: white; padding: 5px; }");

    // Connect button to Qt's quit signal
    QObject::connect(button.get(), &QPushButton::clicked, []()
                     { QApplication::quit(); });

    return button;
}

/**
 * @brief Constructs the full GUI layout and returns it as a smart pointer.
 *        Qt takes ownership of widgets through the layout system, so we release smart pointers accordingly.
 */
std::unique_ptr<QWidget> createGui()
{
    auto window{std::make_unique<QWidget>()};
    window->setWindowTitle("Dark Mode LED GUI");
    window->setFixedSize(400, 250); // Fixed size window

    // Set black background using QPalette
    QPalette palette{window->palette()};
    palette.setColor(QPalette::Window, Qt::black);
    window->setAutoFillBackground(true);
    window->setPalette(palette);

    // Build all UI components
    auto titleLabel{createTitleLabel()};
    auto ledGroup{createLedControls()};
    auto exitButton{createExitButton()};
    auto layout{std::make_unique<QVBoxLayout>()};

    // Add widgets to layout and transfer ownership to Qt
    layout->addWidget(titleLabel.release()); // QLabel released to Qt
    layout->addWidget(ledGroup.release());   // QGroupBox released to Qt
    layout->addWidget(exitButton.get());     // Temporarily used for alignment
    layout->setAlignment(exitButton.get(), Qt::AlignCenter);
    exitButton.release(); // Qt now owns the button

    layout->addStretch();                // Push Exit button upward slightly for better spacing
    window->setLayout(layout.release()); // Layout transferred to Qt

    return window; // Smart pointer returned to be held by main()
}

/**
 * @brief Main entry point. Initializes GPIO, sets up cleanup handler, builds and runs GUI.
 */
int main(int argc, char *argv[])
{
    QApplication app{argc, argv}; // Qt application instance

    try
    {
        setupGpio(); // Initialize all GPIOs for LED output

        // Register a cleanup handler when application is about to quit
        QObject::connect(&app, &QCoreApplication::aboutToQuit, []()
                         {
                             gpioWrite(RED_LED, 0);
                             gpioWrite(GREEN_LED, 0);
                             gpioWrite(BLUE_LED, 0);
                             gpioTerminate(); // Reset all GPIOs and release pigpio resources
                         });

        // Create main window GUI and show it
        std::unique_ptr<QWidget> window{createGui()};
        window->show();

        return app.exec(); // Start the Qt event loop
    }
    catch (const std::exception &ex)
    {
        // Print error and ensure GPIO shutdown if setup failed
        qCritical("Startup Error: %s", ex.what());
        gpioTerminate();
        return 1;
    }
}

<?php
// Old way: https://localhost/index.php?echo=true&img=false&imgDataOutputOnly=false
// Use this: https://localhost/index.php?echo&img
const DEFAULT_WIDTH = 128;
const DEFAULT_HEIGHT = 128;

$info = false;
$source = 0;
$width = -1;
$height = -1;
$loggingLevel = "none"; // "none", "console", "page", "both"
$help = false;

$webPageStart = '
<!DOCTYPE html>
<html>
    <head>
        <title>PHP Test FooBar</title>
    </head>
    <body style="font-family: Arial, sans-serif; margin: 20px;">
        <h1>PHP Image Processing Test</h1>';

$webPageEnd = '
    </body>
</html>';

function logpage($data) {
    if ($GLOBALS['loggingLevel'] === "none" || $GLOBALS['loggingLevel'] === "console") 
        return;
    echo "<p>" . $data . "</p>";
}

function logcon($data) {
    if ($GLOBALS['loggingLevel'] === "none" || $GLOBALS['loggingLevel'] === "page") 
        return;
    $output = $data;
    if (is_array($output))
        $output = implode(',', $output);
    //echo "<script>console.log('->> " . $output . "' );</script>";
    echo "<script>console.log('->> " . htmlspecialchars($output, ENT_QUOTES) . "' );</script>";
}

if (count($_GET) > 0) {
    // Shorter version, check if the parameter exists
    if (isset($_GET['help'])) {
        $help = true;
    }
    if (isset($_GET['info'])) {
        $info = true;
    }
    // if (isset($_GET['source'])) {  
    //     $source = intval($_GET['source']);
    // }
    $source = isset($_GET['source']) ? max(0, min(3, intval($_GET['source']))) : 0;
    // if (isset($_GET['width'])) {
    //     $width = intval($_GET['width']);
    // }
    $width = isset($_GET['width']) ? max(1, min(2000, intval($_GET['width']))) : DEFAULT_WIDTH;
    // if (isset($_GET['height'])) {
    //     $height = intval($_GET['height']);
    // }
    $height = isset($_GET['height']) ? max(1, min(2000, intval($_GET['height']))) : DEFAULT_HEIGHT;
    if (isset($_GET['log'])) {
        $loggingLevel = intval($_GET['log']);
    }
}

if ($help) {
    echo $webPageStart;
    echo '<h2>Help</h2>';
    echo '<p>Use the following GET parameters to control the script:</p>';
    echo '<ul>';
    echo '<li><strong>info</strong>: Show detailed information about processing steps, overrides log</li>';
    echo '<li><strong>source</strong>: Select image source (0, 1, 2, ...).</li>';
    echo '<li><strong>width</strong>: Desired width of the output image.</li>';
    echo '<li><strong>height</strong>: Desired height of the output image.</li>';
    echo '<li><strong>log</strong>: Set logging level (none, console, page, both).</li>';
    echo '</ul>';
    echo '<p>Examples of usage:</p>';
    echo '<ul>';
    echo '<li><code>https://&lt;address&gt;</code></li>';
    echo '<li><code>https://&lt;address&gt;/index.php</code></li>';
    echo '<li><code>?info&width=200</code></li>';
    echo '<li><code>?source=1&width=300&height=120&log=both</code></li>';
    echo '</ul>';
    echo $webPageEnd;
    exit;
}


if ($info)
    $loggingLevel = "both";

if ($info) echo $webPageStart;

logcon("Info is " . ($info ? 'ON' : 'OFF'));    
logcon("width is " . $width);
logcon("height is " . $height);
logcon("source is " . $source);
logcon("loggingLevel is " . $loggingLevel);

// Enhanced image download function with better redirect handling
function downloadImageWithRedirects($url, $maxRedirects = 10, $timeout = 30) {
    $ch = curl_init();
   
    // Basic cURL options
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($ch, CURLOPT_MAXREDIRS, $maxRedirects);
    curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
    curl_setopt($ch, CURLOPT_USERAGENT, 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36');
   
    // SSL options for HTTPS
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
    // Note that the above is unsafe! Test these before deploying in production (they won’t work, but worth a try):
    //curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, true);
    //curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
   
    // Additional headers that some services expect
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Accept: image/webp,image/apng,image/*,*/*;q=0.8',
        'Accept-Language: en-US,en;q=0.9',
        'Accept-Encoding: gzip, deflate',
        'Connection: keep-alive',
        'Upgrade-Insecure-Requests: 1'
    ]);
   
    // Enable verbose output for debugging (remove in production)
    // curl_setopt($ch, CURLOPT_VERBOSE, true);
   
    $imageData = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $finalUrl = curl_getinfo($ch, CURLINFO_EFFECTIVE_URL);
    $redirectCount = curl_getinfo($ch, CURLINFO_REDIRECT_COUNT);
    $totalTime = curl_getinfo($ch, CURLINFO_TOTAL_TIME);
   
    curl_close($ch);
   
    return [
        'data' => $imageData,
        'http_code' => $httpCode,
        'final_url' => $finalUrl,
        'redirect_count' => $redirectCount,
        'total_time' => $totalTime,
        'success' => ($imageData !== false && $httpCode == 200)
    ];
}

// Function to standardize JPEG format
function standardizeJpeg($imageData, $quality = 90) {
    // Create image resource from any supported format
    $sourceImage = imagecreatefromstring($imageData);
   
    if ($sourceImage === false) {
        return false;
    }
   
    // Get dimensions
    $width = imagesx($sourceImage);
    $height = imagesy($sourceImage);
   
    // Create a new true color image (24-bit RGB)
    $standardImage = imagecreatetruecolor($width, $height);
   
    // Ensure high quality resampling
    imagealphablending($standardImage, false);
    imagesavealpha($standardImage, true);
   
    // Copy the image to ensure standard format
    imagecopyresampled(
        $standardImage, $sourceImage,
        0, 0, 0, 0,
        $width, $height, $width, $height
    );
   
    // Output as standard baseline JPEG
    ob_start();
    imagejpeg($standardImage, null, $quality);
    $standardImageData = ob_get_contents();
    ob_end_clean();
   
    // Clean up
    imagedestroy($sourceImage);
    imagedestroy($standardImage);
   
    return $standardImageData;
}

// Image resizing function with standardization
function resizeImageWithCrop($imageData, $targetWidth, $targetHeight, $quality = 90) {
    // First standardize the input image
    $standardImageData = standardizeJpeg($imageData, 95); // Higher quality for intermediate processing
    if ($standardImageData === false) {
        return false;
    }
   
    // Create image resource from standardized data
    $sourceImage = imagecreatefromstring($standardImageData);
   
    if ($sourceImage === false) {
        return false;
    }
   
    // Get original dimensions
    $originalWidth = imagesx($sourceImage);
    $originalHeight = imagesy($sourceImage);
   
    // Calculate aspect ratios
    $originalRatio = $originalWidth / $originalHeight;
    $targetRatio = $targetWidth / $targetHeight;
   
    // Determine crop dimensions
    if ($originalRatio > $targetRatio) {
        // Original is wider - crop width
        $cropHeight = $originalHeight;
        $cropWidth = $originalHeight * $targetRatio;
        $cropX = ($originalWidth - $cropWidth) / 2; // Center horizontally
        $cropY = 0;
    } else {
        // Original is taller - crop height
        $cropWidth = $originalWidth;
        $cropHeight = $originalWidth / $targetRatio;
        $cropX = 0;
        $cropY = ($originalHeight - $cropHeight) / 2; // Center vertically
    }
   
    // Create new image with target dimensions
    $resizedImage = imagecreatetruecolor($targetWidth, $targetHeight);
   
    // Set high quality resampling
    imagealphablending($resizedImage, false);
    imagesavealpha($resizedImage, true);
   
    // Copy and resize the cropped portion
    imagecopyresampled(
        $resizedImage,      // Destination image
        $sourceImage,       // Source image
        0, 0,              // Destination x, y
        $cropX, $cropY,    // Source x, y (crop start)
        $targetWidth, $targetHeight,  // Destination width, height
        $cropWidth, $cropHeight       // Source width, height (crop size)
    );
   
    // Convert back to standardized JPEG
    ob_start();
    imagejpeg($resizedImage, null, $quality);
    $resizedImageData = ob_get_contents();
    ob_end_clean();
   
    // Clean up memory
    imagedestroy($sourceImage);
    imagedestroy($resizedImage);
   
    return $resizedImageData;
}

// Define multiple image sources
$imageSources = [
    '0' => 'https://picsum.photos/', // Small random image
    '1' => 'https://c02.purpledshub.com/uploads/sites/40/2023/08/JI230816Cosmos220-6d9254f-edited-scaled.jpg',
    '2' => 'https://picsum.photos/800/600', // Random image 800x600
    '3' => 'https://picsum.photos/id/237/800/600' // Specific image (dog)    
];

// Select image source (you can make this dynamic via GET parameter)
$selectedSource = $source; //isset($_GET['source']) ? $_GET['source'] : '0';
$suffix = '';

if ($selectedSource == '0') {
    logcon('>>> Selecting suffix...');
    // Build URL with specific dimensions for picsum.photos
    if (($height > -1 && $width > -1) && ($height == $width)) {
       logcon(('>>>>> (1)'));
       $suffix = $width;
    } else if ($width > -1 && $height == -1) {
        logcon(('>>>>> ( 2)'));
        $height = $width;
        $suffix = $width;
    } else if ($height > -1 && $width > -1) {
        logcon(('>>>>> (3)'));
        $suffix = $width . '/' . $height;
    } else if ($height == -1 && $width == -1) {
        logcon(('>>>>> (4)'));
        $width = DEFAULT_WIDTH;
        $height = DEFAULT_HEIGHT;
        $suffix = '128'; // Default size
    }
    else {
        logcon(('>>>>> (5)'));
    }
    $imageUrl = $imageSources['0'] . $suffix; // Request a specific size
} else {
    $imageUrl = isset($imageSources[$selectedSource]) ? $imageSources[$selectedSource] : $imageSources['0'];
}

logcon(' ----- ');
logcon("suffix set to: " . $suffix);
logcon("Selected source: " . $selectedSource);
logcon("Image URL: " . $imageUrl);

//$imageUrl = isset($imageSources[$selectedSource]) ? $imageSources[$selectedSource] : $imageSources['0'];

logpage("Using image source: <strong>$selectedSource</strong>");
logpage("Image URL: <strong>$imageUrl</strong>");

// Download image using enhanced function
$downloadResult = downloadImageWithRedirects($imageUrl, 10, 30);

if ($downloadResult['success']) {
    $imageData = $downloadResult['data'];

    logpage('Image downloaded successfully!');
    logpage('Final URL: ' . $downloadResult['final_url']);
    logpage('Redirects: ' . $downloadResult['redirect_count']);
    logpage('Download time: ' . round($downloadResult['total_time'], 2) . 's');
    logpage('Original image size: ' . strlen($imageData) . ' bytes');

    // Standardize the original image
    $standardizedImage = standardizeJpeg($imageData, 90);
    logpage('Standardized image size: ' . strlen($standardizedImage) . ' bytes');

    // Display original image
    $base64Original = base64_encode($imageData);
    logpage('<h3>Original Image:</h3>');
    if ($info) echo '<img src="data:image/jpeg;base64,' . $base64Original . '" style="max-width: 300px;" alt="Original Image">';
   
    // Display standardized image
    $base64Standardized = base64_encode($standardizedImage);
    logpage('<h3>Standardized Image (Baseline JPEG, 24-bit):</h3>');
    if ($info) echo '<img src="data:image/jpeg;base64,' . $base64Standardized . '" style="max-width: 300px;" alt="Standardized Image">';
   
    // Resize image based on provided width and height (default 128x128)
    $resizedImage = resizeImageWithCrop($imageData, $width, $height, 100);
    if ($resizedImage) {
        logpage('<h3>Resized Image (' . $width . 'x' . $height . '):</h3>');
        $base64Resized4 = base64_encode($resizedImage);
        if ($info) echo '<img src="data:image/jpeg;base64,' . $base64Resized4 . '" alt="Resized Image" style="max-width: 300px;">';
        logpage('Resized image size: ' . strlen($resizedImage) . ' bytes');
    }

    logpage('<h3>Base64 Resized Image Data (' . $width . 'x' . $height . '):</h3>');
    logpage('<textarea style="width:100%; height:200px;">' . $base64Resized4 . '</textarea>');


    if (!$info) {
        logpage('--- Outputting only the resized image data ---', 2);
        // Output only the last resized image data
        echo $base64Resized4;
    }  
   
} else {
    logpage('Failed to download image! HTTP Code: ' . $downloadResult['http_code']);
    logpage('Final URL: ' . $downloadResult['final_url']);
    logpage('Redirects: ' . $downloadResult['redirect_count']);
}

if ($info) echo $webPageEnd;
?>

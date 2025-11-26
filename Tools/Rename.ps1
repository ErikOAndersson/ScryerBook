
# Define the folder path
#$folderPath = "C:\Path\To\Your\Folder"
$folderPath = "."

# Get all files matching the pattern alb_*.jpg
Get-ChildItem -Path $folderPath -Filter "alb_*.jpg" | ForEach-Object {
    # Extract the current file name
    $oldName = $_.Name

    # Build the new name by inserting "_sw" after "alb"
    $newName = $oldName -replace '^alb_', 'alb_sw_'

    # Rename the file
    Rename-Item -Path $_.FullName -NewName $newName
}

Write-Host "Renaming completed!"

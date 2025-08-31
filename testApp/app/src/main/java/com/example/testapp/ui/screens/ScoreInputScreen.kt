package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.testapp.data.User

/**
 * スコア入力画面
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ScoreInputScreen(
    currentUser: User,
    currentUserIndex: Int,
    totalUsers: Int,
    currentScoreValue: String,
    currentScoreDescription: String,
    onScoreValueChange: (String) -> Unit,
    onScoreDescriptionChange: (String) -> Unit,
    onAddScore: () -> Unit,
    onPreviousUser: () -> Unit,
    onNextUser: () -> Unit,
    onNavigateToHistory: () -> Unit,
    canGoToPrevious: Boolean,
    canGoToNext: Boolean,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp)
    ) {
        // ヘッダー
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.primaryContainer
            )
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text(
                            text = "スコア入力",
                            fontSize = 20.sp,
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                        Text(
                            text = "${currentUserIndex + 1} / $totalUsers",
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                    
                    IconButton(
                        onClick = onNavigateToHistory
                    ) {
                        Icon(
                            imageVector = Icons.Default.History,
                            contentDescription = "履歴を見る",
                            tint = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                }
                
                Spacer(modifier = Modifier.height(12.dp))
                
                LinearProgressIndicator(
                    progress = { (currentUserIndex + 1).toFloat() / totalUsers.toFloat() },
                    modifier = Modifier.fillMaxWidth(),
                )
            }
        }
        
        Spacer(modifier = Modifier.height(24.dp))
        
        // 現在のユーザー表示
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = Icons.Default.Person,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.size(32.dp)
                )
                Spacer(modifier = Modifier.width(12.dp))
                Text(
                    text = currentUser.name,
                    fontSize = 24.sp,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
        
        Spacer(modifier = Modifier.height(32.dp))
        
        // スコア入力フィールド
        OutlinedTextField(
            value = currentScoreValue,
            onValueChange = onScoreValueChange,
            label = { Text("スコア") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            leadingIcon = {
                Icon(
                    imageVector = Icons.Default.Star,
                    contentDescription = null
                )
            },
            modifier = Modifier.fillMaxWidth()
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // スコア説明入力フィールド（オプション）
        OutlinedTextField(
            value = currentScoreDescription,
            onValueChange = onScoreDescriptionChange,
            label = { Text("メモ (任意)") },
            leadingIcon = {
                Icon(
                    imageVector = Icons.Default.Description,
                    contentDescription = null
                )
            },
            modifier = Modifier.fillMaxWidth()
        )
        
        Spacer(modifier = Modifier.height(32.dp))
        
        // スコア追加ボタン
        Button(
            onClick = onAddScore,
            enabled = currentScoreValue.isNotBlank() && currentScoreValue.toIntOrNull() != null,
            modifier = Modifier.fillMaxWidth()
        ) {
            Icon(
                imageVector = Icons.Default.Add,
                contentDescription = null
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = "スコアを追加",
                fontSize = 16.sp,
                modifier = Modifier.padding(8.dp)
            )
        }
        
        Spacer(modifier = Modifier.weight(1f))
        
        // ユーザー切り替えボタン
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            OutlinedButton(
                onClick = onPreviousUser,
                enabled = canGoToPrevious,
                modifier = Modifier.weight(1f)
            ) {
                Icon(
                    imageVector = Icons.Default.NavigateBefore,
                    contentDescription = null
                )
                Spacer(modifier = Modifier.width(4.dp))
                Text("前のユーザー")
            }
            
            Spacer(modifier = Modifier.width(16.dp))
            
            OutlinedButton(
                onClick = onNextUser,
                enabled = canGoToNext,
                modifier = Modifier.weight(1f)
            ) {
                Text("次のユーザー")
                Spacer(modifier = Modifier.width(4.dp))
                Icon(
                    imageVector = Icons.Default.NavigateNext,
                    contentDescription = null
                )
            }
        }
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // ヒント
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.5f)
            )
        ) {
            Row(
                modifier = Modifier.padding(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = Icons.Default.Info,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.size(16.dp)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    text = "右上の履歴ボタンから過去のスコアを確認・編集できます",
                    fontSize = 12.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

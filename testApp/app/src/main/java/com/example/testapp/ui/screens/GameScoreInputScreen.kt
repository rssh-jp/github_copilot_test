package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
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
 * ゲームスコア入力画面
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GameScoreInputScreen(
    users: List<User>,
    gameName: String,
    gameScoreInputs: Map<String, String>,
    onGameNameChange: (String) -> Unit,
    onScoreInputChange: (String, String) -> Unit,
    onSaveScores: () -> Unit,
    onCancel: () -> Unit,
    modifier: Modifier = Modifier
) {
    val allScoresEntered = users.all { user ->
        val scoreInput = gameScoreInputs[user.id] ?: ""
        scoreInput.isNotBlank() && scoreInput.toIntOrNull() != null
    }
    
    Column(
        modifier = modifier.fillMaxSize()
    ) {
        // ヘッダー
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.primaryContainer
            )
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    IconButton(onClick = onCancel) {
                        Icon(
                            imageVector = Icons.Default.ArrowBack,
                            contentDescription = "戻る",
                            tint = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                    Text(
                        text = "ゲームスコア入力",
                        fontSize = 24.sp,
                        fontWeight = FontWeight.Bold,
                        color = MaterialTheme.colorScheme.onPrimaryContainer
                    )
                }
            }
        }
        
        Column(
            modifier = Modifier
                .weight(1f)
                .padding(16.dp)
        ) {
            // ゲーム名入力
            OutlinedTextField(
                value = gameName,
                onValueChange = onGameNameChange,
                label = { Text("ゲーム名") },
                leadingIcon = {
                    Icon(
                        imageVector = Icons.Default.SportsEsports,
                        contentDescription = null
                    )
                },
                modifier = Modifier.fillMaxWidth()
            )
            
            Spacer(modifier = Modifier.height(24.dp))
            
            // 説明
            Text(
                text = "各プレイヤーのスコアを入力してください",
                fontSize = 16.sp,
                color = MaterialTheme.colorScheme.onSurface
            )
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // ユーザー毎のスコア入力
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(12.dp),
                modifier = Modifier.weight(1f)
            ) {
                items(users) { user ->
                    val scoreInput = gameScoreInputs[user.id] ?: ""
                    val isError = scoreInput.isNotBlank() && scoreInput.toIntOrNull() == null
                    
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceVariant
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(16.dp)
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Icon(
                                    imageVector = Icons.Default.Person,
                                    contentDescription = null,
                                    tint = MaterialTheme.colorScheme.primary,
                                    modifier = Modifier.size(24.dp)
                                )
                                Spacer(modifier = Modifier.width(12.dp))
                                Text(
                                    text = user.name,
                                    fontSize = 18.sp,
                                    fontWeight = FontWeight.Medium,
                                    modifier = Modifier.weight(1f)
                                )
                            }
                            
                            Spacer(modifier = Modifier.height(8.dp))
                            
                            OutlinedTextField(
                                value = scoreInput,
                                onValueChange = { value ->
                                    onScoreInputChange(user.id, value)
                                },
                                label = { Text("スコア") },
                                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                                leadingIcon = {
                                    Icon(
                                        imageVector = Icons.Default.Star,
                                        contentDescription = null
                                    )
                                },
                                isError = isError,
                                supportingText = if (isError) {
                                    { Text("数値を入力してください") }
                                } else null,
                                modifier = Modifier.fillMaxWidth()
                            )
                        }
                    }
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // 保存ボタン
            Button(
                onClick = onSaveScores,
                enabled = allScoresEntered && gameName.isNotBlank(),
                modifier = Modifier.fillMaxWidth()
            ) {
                Icon(
                    imageVector = Icons.Default.Save,
                    contentDescription = null
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(
                    text = "スコアを保存",
                    fontSize = 16.sp,
                    modifier = Modifier.padding(8.dp)
                )
            }
            
            Spacer(modifier = Modifier.height(8.dp))
            
            // キャンセルボタン
            OutlinedButton(
                onClick = onCancel,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("キャンセル")
            }
            
            // 入力状況の表示
            if (!allScoresEntered) {
                Spacer(modifier = Modifier.height(16.dp))
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
                            text = "全てのプレイヤーのスコアを入力してください",
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }
        }
    }
}
